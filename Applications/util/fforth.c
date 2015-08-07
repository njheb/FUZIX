#define dummy /*
# fforth © 2015 David Given
# This program is available under the terms of the 2-clause BSD license.
# The full text is available here: http://opensource.org/licenses/BSD-2-Clause
#
# fforth is a small Forth written in portable C. It should Just Compile on
# most Unixy platforms. It's intended as a scripting language for the Fuzix
# operating system.
#
# It's probably a bit weird --- I'm using the ANS Forth reference here:
# http://lars.nocrew.org/dpans/dpans6.htm
# ...but I've been playing fast and loose with the standard.
#
# Peculiarities include:
# 
# Note! This program looks weird. That's because it's a shell script *and* a C
# file. (And an Awk script.) The awk file will autogenerate the Forth dictionary
# and precompiled words in the C source, which is just too fragile to do by
# hand.
# 
# //@W: marks a dictionary entry. This will get updated in place to form a linked
# list.
#
# //@C: cheesy™ precompiled Forth. Put semi-Forth code on the following comment
# lines; the line immediately afterwards will be updated to contain the byte-
# compiled version. Don't put a trailing semicolon.
#
# C compilation options:
#
#   -DFAST             don't bounds check the stack (smaller, faster code)
#
# No evil was harmed in the making of this file. Probably.

set -e
trap 'rm /tmp/$$.words' EXIT

# Get the list of words (for forward declaration).
awk -f- $0 >/tmp/$$.words <<EOF
	/\/\/@W$/ {
		n = \$2
		sub(/,/, " ", n)
		print("static cdefn_t " n ";")
	}
EOF

# Now actually edit the source file.
awk -f- $0 > $0.new <<EOF
	BEGIN {
		lastword = "NULL"

		ord_table = ""
		for (i = 0; i < 256; i++)
			ord_table = ord_table sprintf("%c", i)
	}

	function ord(s) {
		return index(ord_table, s) - 1
	}

	/\/\/@EXPORT}\$/ {
		print "//@EXPORT{"
		while ((getline line < "/tmp/$$.words") > 0)
			print "" line
		close("/tmp/$$.words")
		print "//@EXPORT}"
	}
	/\/\/@EXPORT{\$/, /\/\/@EXPORT}\$/ { next }


	/\/\/@W\$/ {
		\$5 = lastword ","

		printf("%s %-19s %-15s %-13s %-17s",
			\$1, \$2, \$3, \$4, \$5)

		payload = ""
		for (i=6; i<=NF; i++)
			printf(" %s", \$i)
		printf("\n")

		wordname = \$4
		sub(/^"/, "", wordname)
		sub(/",$/, "", wordname)
		sub(/\\\\./, "&", wordname)
		words[wordname] = \$2

		lastword = "&" \$2
		sub(/,/, "", lastword)

		next
	}

	/\/\/@E$/ {
		n = \$2
		sub(/,/, " ", n)
		printf("static " \$2 " " \$3 " = (defn_t*) " lastword "; //@E\n")
		next
	}

	function push(n) {
		stack[sp++] = n
	}

	function pop() {
		return stack[--sp]
	}

	function comma(s) {
		if (s !~ /,$/)
			s = s ","
		bytecode[pc++] = s
	}

	function compile(n) {
		if (n == "IF")
		{
			comma("&branch0_word")
			push(pc)
			comma(0)
			return
		}
		if (n == "ELSE")
		{
			elsejump = pop()
			comma("&branch_word")
			push(pc)
			comma(0)

			bytecode[elsejump] = "(&" word ".payload[0] + " pc "),"
			return
		}
		if (n == "THEN")
		{
			bytecode[pop()] = "(&" word ".payload[0] + " pc "),"
			return
		}
			
		if (n == "BEGIN")
		{
			push(pc)
			return
		}
		if (n == "AGAIN")
		{
			comma("&branch_word")
			comma("(&" word ".payload[0] + " pop() "),")
			return
		}
		if (n == "UNTIL")
		{
			comma("&branch0_word")
			comma("(&" word ".payload[0] + " pop() "),")
			return
		}
		if (n == "WHILE")
		{
			comma("&branch0_word")
			push(pc)
			comma(0)
			return
		}
		if (n == "REPEAT")
		{
			whilefalse = pop()
			comma("&branch_word")
			comma("(&" word ".payload[0] + " pop() "),")
			bytecode[whilefalse] = "(&" word ".payload[0] + " pc "),"
			return
		}

		if (n == "RECURSE")
		{
			comma("&" word)
			return
		}

		if (n ~ /^\[.*]$/)
		{
			sub(/^\\[/, "", n)
			sub(/]$/, "", n)
			comma("(" n ")")
			return
		}

		wordsym = words[n]
		if (wordsym == "")
		{
			if (n ~ /-?[0-9]/)
			{
				comma("&lit_word,")
				comma(n ",")
				return
			}

			printf("Unrecognised word '%s' while defining '%s'\n", n, wordstring) > "/dev/stderr"
			exit(1)
		}
		comma("&" wordsym)
	}

	/^\/\/@C/ {
		print

		wordstring = \$2

		word = ""
		for (i=1; i<=length(wordstring); i++)
		{
			c = substr(wordstring, i, 1)
			if (c ~ /[A-Za-z_$]/)
				word = word c
			else
				word = word sprintf("_%02x_", ord(c))
		}
		word = tolower(word) "_word"

		sub(/\\\\/, "\\\\\\\\", wordstring)
		sub(/"/, "\\\\\\"", wordstring)

		immediate = (\$3 == "IMMEDIATE")
		hidden = (\$3 == "HIDDEN")
		sp = 0
		pc = 0

		# (Yes, this is supposed to consume and not print one extra line.)
		while (getline)
		{
			if (\$1 != "//")
			{
				# Consume and do not print.
				break
			}
			print 
			
			for (i=2; i<=NF; i++)
			{
				if (\$i == "\\\\")
					break;
				compile(\$i)
			}
		}

		if (immediate)
			printf("IMM( ")
		else
			printf("COM( ")
		printf("%s, codeword, \"%s\", %s, ", word, hidden ? "" : wordstring, lastword)
		for (i = 0; i < pc; i++)
			printf("(void*)%s ", bytecode[i])
		printf("(void*)&exit_word )\n")
		lastword = "&" word
		words[wordstring] = word ","

		next
	}

	{
		print
	}
EOF

# Replace the old file with the new.

mv $0 $0.old
mv $0.new $0

echo "Updated!"

exit 0
*/

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if INTPTR_MAX == INT16_MAX
	typedef int16_t cell_t;
	typedef uint16_t ucell_t;
	typedef int32_t pair_t;
	typedef uint32_t upair_t;
#elif INTPTR_MAX == INT32_MAX
	typedef int32_t cell_t;
	typedef uint32_t ucell_t;
	typedef int64_t pair_t;
	typedef uint64_t upair_t;
#elif INTPTR_MAX == INT64_MAX
	typedef int64_t cell_t;
	typedef uint64_t ucell_t;
	/* This works on gcc and is useful for debugging; I'm not really expecting
	 * people will be using fforth for real on 64-bit platforms. */
	typedef __int128_t pair_t;
	typedef __uint128_t upair_t;
#else
	#error "Don't understand the size of your platform!"
#endif

typedef struct definition defn_t;
typedef const struct definition cdefn_t;

static jmp_buf onerror;

#define MAX_LINE_LENGTH 160
#define ALLOCATION_CHUNK_SIZE 128
#define CELL sizeof(cell_t)

#define DSTACKSIZE 64
static cell_t dstack[DSTACKSIZE];
static cell_t* dsp;

#define RSTACKSIZE 64
static cell_t rstack[RSTACKSIZE];
static cell_t* rsp;

static int input_fd;
static char input_buffer[MAX_LINE_LENGTH];
static cell_t in_arrow;
static cell_t base = 10;
static cell_t state = false;

static defn_t** pc;
static defn_t* latest; /* Most recent word on dictionary */
static cdefn_t* last;   /* Last of the built-in words */

static uint8_t* here;
static uint8_t* here_top;

typedef void code_fn(cdefn_t* w);
static void align_cb(cdefn_t* w);

#define FL_IMM 0x80

#define CONST_STRING(n, v) \
	static const char n[sizeof(v)-1] = v

struct fstring
{
	uint8_t len;
	char data[];
};

struct definition
{
	code_fn* code;
	struct fstring* name;
	cdefn_t* next;
	void* payload[];
};

static void strerr(const char* s)
{
	write(2, s, strlen(s));
}

static void panic(const char* message)
{
	strerr("panic: ");
	strerr(message);
	strerr("\n");
	longjmp(onerror, 1);
}

#if !defined FAST
static void dadjust(int delta)
{
	dsp -= delta;
	if (dsp <= dstack)
		panic("data stack overflow");
	if (dsp > dstack+DSTACKSIZE)
		panic("data stack underflow");
}

static void radjust(int delta)
{
	rsp -= delta;
	if (rsp <= rstack)
		panic("return stack overflow");
	if (rsp > rstack+RSTACKSIZE)
		panic("return stack underflow");
}

static void dpush(cell_t val)
{
	dadjust(1);
	*dsp = val;
}

static cell_t dpop(void)
{
	cell_t v = *dsp;
	dadjust(-1);
	return v;
}

static cell_t* daddr(int count)
{
	cell_t* ptr = dsp + count;
	if (ptr > dstack+DSTACKSIZE)
		panic("data stack underflow");
	return ptr;
}

static void rpush(cell_t val)
{
	radjust(1);
	*rsp = val;
}

static cell_t rpop(void)
{
	cell_t v = *rsp;
	radjust(-1);
	return v;
}

static cell_t* raddr(int count)
{
	cell_t* ptr = rsp + count;
	if (ptr >= rstack+RSTACKSIZE)
		panic("return stack underflow");
	return ptr;
}

#else
static inline void dadjust(cell_t val, int delta) { dsp -= delta; }
static inline void radjust(cell_t val, int delta) { rsp -= delta; }
static inline void dpush(cell_t val) { *--dsp = val; }
static inline cell_t dpop(void) { return *dsp++; }
static inline cell_t daddr(int count) { return dsp+count; }
static inline void rpush(cell_t val) { *--rsp = val; }
static inline cell_t rpop(void) { return *rsp++; }
static inline cell_t raddr(int count) { return rsp+count; }
#endif

static pair_t readpair(ucell_t* ptr)
{
	upair_t p = ptr[0];
	p <<= sizeof(cell_t)*8;
	p |= ptr[1];
	return p;
}

static void writepair(ucell_t* ptr, upair_t p)
{
	ptr[1] = p;
	ptr[0] = p >> (sizeof(cell_t)*8);
}

static pair_t dpopd(void)
{
	pair_t v = readpair((ucell_t*) dsp);
	dadjust(-2);
	return v;
}

static void dpushd(upair_t p)
{
	dadjust(2);
	writepair((ucell_t*) dsp, p);
}

static void dpushbool(bool b)
{
	dpush(b ? -1 : 0);
}

static void* ensure_workspace(size_t length)
{
	uint8_t* p = here + length;

	if (p > here_top)
	{
		uint8_t* newtop = sbrk(ALLOCATION_CHUNK_SIZE);
		if (newtop != here_top)
			panic("non-contiguous sbrk memory");
		here_top = newtop + ALLOCATION_CHUNK_SIZE;
	}

	return here;
}

static void* claim_workspace(size_t length)
{
	uint8_t* p = ensure_workspace(length);
	here += length;
	return p;
}

/* Note --- this only works properly on word names, not general counted
 * strings, because it ignores the top bit of the length (used in the
 * dictionary as a flag). */
static int fstreq(const struct fstring* f1, const struct fstring* f2)
{
	int len1 = f1->len & 0x7f;
	int len2 = f2->len & 0x7f;
	if (len1 != len2)
		return 0;
	return (memcmp(f1->data, f2->data, len1) == 0);
}

/* Forward declarations of words go here --- do not edit.*/
//@EXPORT{
static cdefn_t E_fnf_word ;
static cdefn_t _O_RDONLY_word ;
static cdefn_t _O_RDWR_word ;
static cdefn_t _O_WRONLY_word ;
static cdefn_t _close_word ;
static cdefn_t _create_word ;
static cdefn_t _exit_word ;
static cdefn_t _input_fd_word ;
static cdefn_t _open_word ;
static cdefn_t _read_word ;
static cdefn_t _stderr_word ;
static cdefn_t _stdin_word ;
static cdefn_t _stdout_word ;
static cdefn_t _write_word ;
static cdefn_t a_number_word ;
static cdefn_t abort_word ;
static cdefn_t abs_word ;
static cdefn_t accept_word ;
static cdefn_t add_one_word ;
static cdefn_t add_word ;
static cdefn_t align_word ;
static cdefn_t allot_word ;
static cdefn_t and_word ;
static cdefn_t arrow_r_word ;
static cdefn_t at_word ;
static cdefn_t base_word ;
static cdefn_t branch0_word ;
static cdefn_t branch_word ;
static cdefn_t c_at_word ;
static cdefn_t c_pling_word ;
static cdefn_t cell_word ;
static cdefn_t close_sq_word ;
static cdefn_t dabs_word ;
static cdefn_t drop_word ;
static cdefn_t dup_word ;
static cdefn_t equals0_word ;
static cdefn_t equals_word ;
static cdefn_t execute_word ;
static cdefn_t exit_word ;
static cdefn_t fill_word ;
static cdefn_t find_word ;
static cdefn_t fm_mod_word ;
static cdefn_t ge_word ;
static cdefn_t gt_word ;
static cdefn_t here_word ;
static cdefn_t in_arrow_word ;
static cdefn_t invert_word ;
static cdefn_t latest_word ;
static cdefn_t le_word ;
static cdefn_t less0_word ;
static cdefn_t lit_word ;
static cdefn_t lshift_word ;
static cdefn_t lt_word ;
static cdefn_t m_one_word ;
static cdefn_t m_star_word ;
static cdefn_t more0_word ;
static cdefn_t mul_word ;
static cdefn_t not_equals_word ;
static cdefn_t notequals0_word ;
static cdefn_t one_word ;
static cdefn_t or_word ;
static cdefn_t over_word ;
static cdefn_t pad_word ;
static cdefn_t pick_word ;
static cdefn_t pling_word ;
static cdefn_t q_dup_word ;
static cdefn_t r_arrow_word ;
static cdefn_t rdrop_word ;
static cdefn_t rot_word ;
static cdefn_t rpick_word ;
static cdefn_t rshift_word ;
static cdefn_t rsp0_word ;
static cdefn_t rsp_at_word ;
static cdefn_t rsp_pling_word ;
static cdefn_t rsshift_word ;
static cdefn_t source_word ;
static cdefn_t sp0_word ;
static cdefn_t sp_at_word ;
static cdefn_t sp_pling_word ;
static cdefn_t state_word ;
static cdefn_t sub_one_word ;
static cdefn_t sub_word ;
static cdefn_t swap_word ;
static cdefn_t t_drop_word ;
static cdefn_t t_dup_word ;
static cdefn_t t_over_word ;
static cdefn_t t_swap_word ;
static cdefn_t tuck_word ;
static cdefn_t two_word ;
static cdefn_t u_lt_word ;
static cdefn_t u_m_star_word ;
static cdefn_t um_mod ;
static cdefn_t word_word ;
static cdefn_t xor_word ;
static cdefn_t zero_word ;
static cdefn_t immediate_word ;
static cdefn_t open_sq_word ;
//@EXPORT}

/* ======================================================================= */
/*                                  WORDS                                  */
/* ======================================================================= */

static void codeword(cdefn_t* w) { rpush((cell_t) pc); pc = (void*) &w->payload[0]; }
static void dataword(cdefn_t* w) { dpush((cell_t) &w->payload[0]); }
static void rvarword(cdefn_t* w) { dpush((cell_t) w->payload[0]); }
static void r2varword(cdefn_t* w) { dpush((cell_t) w->payload[0]); dpush((cell_t) w->payload[1]); }
static void wvarword(defn_t* w) { w->payload[0] = (void*) dpop(); }
static void rivarword(cdefn_t* w) { dpush(*(cell_t*) w->payload[0]); }
static void wivarword(cdefn_t* w) { *(cell_t*)(w->payload[0]) = dpop(); }

static void _readwrite_cb(cdefn_t* w)
{
	size_t len = dpop();
	void* ptr = (void*)dpop();
	int fd = dpop();
	int (*func)(int fd, void* ptr, size_t size) = (void*) *w->payload;

	dpush(func(fd, ptr, len));
}

static void _open_cb(cdefn_t* w)
{
	int flags = dpop();
	const char* filename = (void*)dpop();
	dpush(open(filename, flags));
}

static void accept_cb(cdefn_t* w)
{
	cell_t max = dpop();
	char* addr = (char*)dpop();
	int len = 0;

	while (len < max)
	{
		char c;
		if (read(input_fd, &c, 1) <= 0)
		{
			if (len == 0)
				len = -1;
			break;
		}
		if (c == '\n')
			break;

		addr[len++] = c;
	}
	dpush(len);
}

static void fill_cb(cdefn_t* w)
{
	cell_t c = dpop();
	cell_t len = dpop();
	void* ptr = (void*) dpop();

	memset(ptr, c, len);
}

static void immediate_cb(cdefn_t* w)
{
	latest->name->len |= FL_IMM;
}

static bool is_delimiter(int c, int delimiter)
{
	if (c == delimiter)
		return true;
	if ((delimiter == ' ') && (c < 32))
		return true;
	return false;
}

static void skip_ws(int delimiter)
{
	while (in_arrow < MAX_LINE_LENGTH)
	{
		int c = input_buffer[in_arrow];
		if (!is_delimiter(c, delimiter))
			break;
		in_arrow++;
	}
}

static void word_cb(cdefn_t* w)
{
	int delimiter = dpop();
	struct fstring* fs = ensure_workspace(MAX_LINE_LENGTH);
	int count = 0;

	skip_ws(delimiter);
	if (in_arrow != MAX_LINE_LENGTH)
	{
		while (in_arrow < MAX_LINE_LENGTH)
		{
			int c = input_buffer[in_arrow];
			if (is_delimiter(c, delimiter))
				break;
			fs->data[count] = c;
			count++;
			in_arrow++;
		}
	}
	skip_ws(delimiter);

	#if 0
		strerr("<");
		write(2, &fs->data[0], count);
		strerr(">");
	#endif

	fs->len = count;
	dpush((cell_t) fs);
}

static void _create_cb(cdefn_t* w)
{
	/* The name of the word is passed on the stack. */

	struct fstring* name = (void*)dpop();

	/* Create the word header. */

	defn_t* defn = claim_workspace(sizeof(defn_t));
	defn->code = dataword;
	defn->name = name;
	defn->next = latest;
	#if 0
		printf("[defined ");
		fwrite(&defn->name->data[0], 1, defn->name->len & 0x7f, stdout);
		printf("]\n");
	#endif
	latest = defn;
}

static void find_cb(cdefn_t* w)
{
	struct fstring* name = (void*) dpop();
	cdefn_t* current = latest;

	#if 0
		printf("[find ");
		fwrite(&name->data[0], 1, name->len & 0x7f, stdout);
		printf("]\n");
	#endif

	while (current)
	{
		if (current->name && fstreq(name, current->name))
		{
			dpush((cell_t) current);
			dpush((current->name->len & FL_IMM) ? 1 : -1);
			return;
		}
		current = current->next;
	}

	dpush((cell_t) name);
	dpush(0);
}

static unsigned get_digit(char p)
{
	if (p >= 'a')
		return 10 + p - 'a';
	if (p >= 'A')
		return 10 + p - 'A';
	return p - '0';
}

/* This is Forth's rather complicated number parse utility.
 * ( ud c-addr len -- ud' c-addr' len' )
 * Digits are parsed according to base and added to the accumulator ud.
 * Signs are not supported.
 */
static void a_number_cb(cdefn_t* w)
{
	int len = dpop();
	char* addr = (void*) dpop();
	ucell_t ud = dpop();

	while (len > 0)
	{
		unsigned int d = get_digit(*addr);
		if (d >= base)
			break;
		ud = (ud * base) + d;

		len--;
		addr++;
	}

	dpush(ud);
	dpush((cell_t) addr);
	dpush(len);
}

static void rot_cb(cdefn_t* w)
{
	cell_t x3 = dpop();
	cell_t x2 = dpop();
	cell_t x1 = dpop();
	dpush(x2);
	dpush(x3);
	dpush(x1);
}

static void swap_cb(cdefn_t* w)
{
	cell_t x2 = dpop();
	cell_t x1 = dpop();
	dpush(x2);
	dpush(x1);
}

static void t_swap_cb(cdefn_t* w)
{
	cell_t x4 = dpop();
	cell_t x3 = dpop();
	cell_t x2 = dpop();
	cell_t x1 = dpop();
	dpush(x3);
	dpush(x4);
	dpush(x1);
	dpush(x2);
}

static void execute_cb(cdefn_t* w)
{
	cdefn_t* p = (void*) dpop();
	#if 0
		printf("[execute ");
		fwrite(&p->name->data[0], 1, p->name->len & 0x7f, stdout);
		printf("]\n");
	#endif
	p->code(p);
}

static void abs_cb(cdefn_t* w)
{
	cell_t d = dpop();
	if (d < 0)
		d = -d;
	dpush(d);
}

static void dabs_cb(cdefn_t* w)
{
	pair_t d = dpopd();
	if (d < 0)
		d = -d;
	dpushd(d);
}

static void tuck_cb(cdefn_t* w)
{
	cell_t x2 = dpop();
	cell_t x1 = dpop();
	dpush(x2);
	dpush(x1);
	dpush(x2);
}

static void fm_mod_cb(cdefn_t* w)
{
	cell_t den = dpop();
	pair_t num = dpopd();
	cell_t q = num / den;
	cell_t r = num % den;

	if ((num^den) <= 0)
	{
		if (r)
		{
			q -= 1;
			r += den;
		}
    }

	dpush(r);
	dpush(q);
}

static void um_mod_cb(cdefn_t* w)
{
	ucell_t den = dpop();
	upair_t num = dpopd();
	ucell_t q = num / den;
	ucell_t r = num % den;
	dpush(r);
	dpush(q);
}

static void E_fnf_cb(cdefn_t* w)      { panic("file not found"); }
static void _close_cb(cdefn_t* w)     { dpush(close(dpop())); }
static void _exit_cb(cdefn_t* w)      { exit(dpop()); }
static void abort_cb(cdefn_t* w)      { longjmp(onerror, 1); }
static void add_cb(cdefn_t* w)        { dpush(dpop() + dpop()); }
static void align_cb(cdefn_t* w)      { claim_workspace((CELL - (cell_t)here) & (CELL-1)); }
static void allot_cb(cdefn_t* w)      { claim_workspace(dpop()); }
static void and_cb(cdefn_t* w)        { dpush(dpop() & dpop()); }
static void arrow_r_cb(cdefn_t* w)    { rpush(dpop()); }
static void at_cb(cdefn_t* w)         { dpush(*(cell_t*)dpop()); }
static void branch_cb(cdefn_t* w)     { pc = (void*) *pc; }
static void branchif_cb(cdefn_t* w)   { if (dpop() == (cell_t)*w->payload) pc = (void*)*pc; else pc++; }
static void c_at_cb(cdefn_t* w)       { dpush(*(uint8_t*)dpop()); }
static void c_pling_cb(cdefn_t* w)    { uint8_t* p = (uint8_t*)dpop(); *p = dpop(); }
static void close_sq_cb(cdefn_t* w)   { state = 1; }
static void drop_cb(cdefn_t* w)       { dadjust(-1); }
static void rdrop_cb(cdefn_t* w)      { radjust(-1); }
static void equals0_cb(cdefn_t* w)    { dpushbool(dpop() == 0); }
static void equals_cb(cdefn_t* w)     { dpushbool(dpop() == dpop()); }
static void exit_cb(cdefn_t* w)       { pc = (void*)rpop(); }
static void ge_cb(cdefn_t* w)         { cell_t a = dpop(); cell_t b = dpop(); dpushbool(b >= a); }
static void gt_cb(cdefn_t* w)         { cell_t a = dpop(); cell_t b = dpop(); dpushbool(b > a); }
static void increment_cb(cdefn_t* w)  { dpush(dpop() + (cell_t)w->payload[0]); }
static void invert_cb(cdefn_t* w)     { dpush(~dpop()); }
static void le_cb(cdefn_t* w)         { cell_t a = dpop(); cell_t b = dpop(); dpushbool(b <= a); }
static void less0_cb(cdefn_t* w)      { dpushbool(dpop() < 0); }
static void lit_cb(cdefn_t* w)        { dpush((cell_t) *pc++); }
static void lshift_cb(cdefn_t* w)     { cell_t u = dpop(); ucell_t a = dpop(); dpush(a << u); }
static void lt_cb(cdefn_t* w)         { cell_t a = dpop(); cell_t b = dpop(); dpushbool(b < a); }
static void m_star_cb(cdefn_t* w)     { dpushd((pair_t)dpop() * (pair_t)dpop()); }
static void more0_cb(cdefn_t* w)      { dpushbool(dpop() > 0); }
static void mul_cb(cdefn_t* w)        { dpush(dpop() * dpop()); }
static void not_equals_cb(cdefn_t* w) { dpushbool(dpop() != dpop()); }
static void notequals0_cb(cdefn_t* w) { dpushbool(dpop() != 0); }
static void open_sq_cb(cdefn_t* w)    { state = 0; }
static void or_cb(cdefn_t* w)         { dpush(dpop() | dpop()); }
static void peekcon_cb(cdefn_t* w)    { dpush(*daddr((cell_t) *w->payload)); }
static void pokecon_cb(cdefn_t* w)    { cell_t v = dpop(); *daddr((cell_t) *w->payload) = v; }
static void peekcon2_cb(cdefn_t* w)   { peekcon_cb(w); peekcon_cb(w); }
static void pick_cb(cdefn_t* w)       { dpush(*daddr(dpop())); }
static void pling_cb(cdefn_t* w)      { cell_t* p = (cell_t*)dpop(); *p = dpop(); }
static void q_dup_cb(cdefn_t* w)      { cell_t a = *daddr(0); if (a) dpush(a); }
static void r_arrow_cb(cdefn_t* w)    { dpush(rpop()); }
static void rpeekcon_cb(cdefn_t* w)   { dpush(*raddr((cell_t) *w->payload)); }
static void rpokecon_cb(cdefn_t* w)   { cell_t v = dpop(); *raddr((cell_t) *w->payload) = v; }
static void rpick_cb(cdefn_t* w)      { dpush(*raddr(dpop())); }
static void rshift_cb(cdefn_t* w)     { cell_t u = dpop(); ucell_t a = dpop(); dpush(a >> u); }
static void rsshift_cb(cdefn_t* w)    { dpush(dpop() >> 1); }
static void sub_cb(cdefn_t* w)        { cell_t a = dpop(); cell_t b = dpop(); dpush(b - a); }
static void t_drop_cb(cdefn_t* w)     { dadjust(-2); }
static void u_lt_cb(cdefn_t* w)       { ucell_t a = dpop(); ucell_t b = dpop(); dpushbool(b < a); }
static void u_m_star_cb(cdefn_t* w)   { dpushd((upair_t)(ucell_t)dpop() * (upair_t)(ucell_t)dpop()); }
static void xor_cb(cdefn_t* w)        { dpush(dpop() ^ dpop()); }

#define WORD(w, c, n, l, f, p...) \
	struct fstring_##w { uint8_t len; char data[sizeof(n)-1]; }; \
	static struct fstring_##w w##_name = {(sizeof(n)-1) | f, n}; \
	static cdefn_t w = { c, (struct fstring*) &w##_name, l, { p } };

#define COM(w, c, n, l, p...) WORD(w, c, n, l, 0, p)
#define IMM(w, c, n, l, p...) WORD(w, c, n, l, FL_IMM, p)

/* A list of words go here. To add a word, add a new entry and run this file as
 * a shell script. The link field will be set correctly.
 * BEWARE: these lines are parsed using whitespace. LEAVE EXACTLY AS IS.*/
//@WORDLIST
COM( E_fnf_word,         E_fnf_cb,       "E_fnf",      NULL,             (void*)0 ) //@W
COM( _O_RDONLY_word,     rvarword,       "O_RDONLY",   &E_fnf_word,      (void*)O_RDONLY ) //@W
COM( _O_RDWR_word,       rvarword,       "O_RDWR",     &_O_RDONLY_word,  (void*)O_RDWR ) //@W
COM( _O_WRONLY_word,     rvarword,       "O_WRONLY",   &_O_RDWR_word,    (void*)O_WRONLY ) //@W
COM( _close_word,        _close_cb,      "_close",     &_O_WRONLY_word,  ) //@W
COM( _create_word,       _create_cb,     "",           &_close_word,     ) //@W
COM( _exit_word,         _exit_cb,       "_exit",      &_create_word,    ) //@W
COM( _input_fd_word,     rvarword,       "_input_fd",  &_exit_word,      &input_fd ) //@W
COM( _open_word,         _open_cb,       "_open",      &_input_fd_word,  ) //@W
COM( _read_word,         _readwrite_cb,  "_read",      &_open_word,      &read ) //@W
COM( _stderr_word,       rvarword,       "_stderr",    &_read_word,      (void*)2 ) //@W
COM( _stdin_word,        rvarword,       "_stdin",     &_stderr_word,    (void*)0 ) //@W
COM( _stdout_word,       rvarword,       "_stdout",    &_stdin_word,     (void*)1 ) //@W
COM( _write_word,        _readwrite_cb,  "_write",     &_stdout_word,    &write ) //@W
COM( a_number_word,      a_number_cb,    ">NUMBER",    &_write_word,     ) //@W
COM( abort_word,         abort_cb,       "ABORT",      &a_number_word,   ) //@W
COM( abs_word,           abs_cb,         "ABS",        &abort_word,      ) //@W
COM( accept_word,        accept_cb,      "ACCEPT",     &abs_word,        ) //@W
COM( add_one_word,       increment_cb,   "1+",         &accept_word,     (void*)1 ) //@W
COM( add_word,           add_cb,         "+",          &add_one_word,    ) //@W
COM( align_word,         align_cb,       "ALIGN",      &add_word,        ) //@W
COM( allot_word,         allot_cb,       "ALLOT",      &align_word,      ) //@W
COM( and_word,           and_cb,         "AND",        &allot_word,      ) //@W
COM( arrow_r_word,       arrow_r_cb,     ">R",         &and_word,        ) //@W
COM( at_word,            at_cb,          "@",          &arrow_r_word,    ) //@W
COM( base_word,          rvarword,       "BASE",       &at_word,         &base ) //@W
COM( branch0_word,       branchif_cb,    "0BRANCH",    &base_word,       (void*)0 ) //@W
COM( branch_word,        branch_cb,      "BRANCH",     &branch0_word,    ) //@W
COM( c_at_word,          c_at_cb,        "C@",         &branch_word,     ) //@W
COM( c_pling_word,       c_pling_cb,     "C!",         &c_at_word,       ) //@W
COM( cell_word,          rvarword,       "CELL",       &c_pling_word,    (void*)CELL ) //@W
COM( close_sq_word,      close_sq_cb,    "]",          &cell_word,       ) //@W
COM( dabs_word,          dabs_cb,        "DABS",       &close_sq_word,   ) //@W
COM( drop_word,          drop_cb,        "DROP",       &dabs_word,       ) //@W
COM( dup_word,           peekcon_cb,     "DUP",        &drop_word,       (void*)0 ) //@W
COM( equals0_word,       equals0_cb,     "0=",         &dup_word,        ) //@W
COM( equals_word,        equals_cb,      "=",          &equals0_word,    ) //@W
COM( execute_word,       execute_cb,     "EXECUTE",    &equals_word,     ) //@W
COM( exit_word,          exit_cb,        "EXIT",       &execute_word,    ) //@W
COM( fill_word,          fill_cb,        "FILL",       &exit_word,       ) //@W
COM( find_word,          find_cb,        "FIND",       &fill_word,       ) //@W
COM( fm_mod_word,        fm_mod_cb,      "FM/MOD",     &find_word,       ) //@W
COM( ge_word,            ge_cb,          ">=",         &fm_mod_word,     ) //@W
COM( gt_word,            gt_cb,          ">",          &ge_word,         ) //@W
COM( here_word,          rivarword,      "HERE",       &gt_word,         &here ) //@W
COM( in_arrow_word,      rvarword,       ">IN",        &here_word,       &in_arrow ) //@W
COM( invert_word,        invert_cb,      "INVERT",     &in_arrow_word,   ) //@W
COM( latest_word,        rvarword,       "LATEST",     &invert_word,     &latest ) //@W
COM( le_word,            le_cb,          "<=",         &latest_word,     ) //@W
COM( less0_word,         less0_cb,       "0<",         &le_word,         ) //@W
COM( lit_word,           lit_cb,         "LIT",        &less0_word,      ) //@W
COM( lshift_word,        lshift_cb,      "LSHIFT",     &lit_word,        ) //@W
COM( lt_word,            lt_cb,          "<",          &lshift_word,     ) //@W
COM( m_one_word,         rvarword,       "-1",         &lt_word,         (void*)-1 ) //@W
COM( m_star_word,        m_star_cb,      "M*",         &m_one_word,      ) //@W
COM( more0_word,         more0_cb,       "0>",         &m_star_word,     ) //@W
COM( mul_word,           mul_cb,         "*",          &more0_word,      ) //@W
COM( not_equals_word,    not_equals_cb,  "<>",         &mul_word,        ) //@W
COM( notequals0_word,    notequals0_cb,  "0<>",        &not_equals_word, ) //@W
COM( one_word,           rvarword,       "1",          &notequals0_word, (void*)1 ) //@W
COM( or_word,            or_cb,          "OR",         &one_word,        ) //@W
COM( over_word,          peekcon_cb,     "OVER",       &or_word,         (void*)1 ) //@W
COM( pad_word,           rvarword,       "PAD",        &over_word,       &here ) //@W
COM( pick_word,          pick_cb,        "PICK",       &pad_word,        ) //@W
COM( pling_word,         pling_cb,       "!",          &pick_word,       ) //@W
COM( q_dup_word,         q_dup_cb,       "?DUP",       &pling_word,      ) //@W
COM( r_arrow_word,       r_arrow_cb,     "R>",         &q_dup_word,      ) //@W
COM( rdrop_word,         rdrop_cb,       "RDROP",      &r_arrow_word,    ) //@W
COM( rot_word,           rot_cb,         "ROT",        &rdrop_word,      ) //@W
COM( rpick_word,         rpick_cb,       "RPICK",      &rot_word,        ) //@W
COM( rshift_word,        rshift_cb,      "RSHIFT",     &rpick_word,      ) //@W
COM( rsp0_word,          rvarword,       "RSP0",       &rshift_word,     rstack+RSTACKSIZE ) //@W
COM( rsp_at_word,        rivarword,      "RSP@",       &rsp0_word,       &rsp ) //@W
COM( rsp_pling_word,     wivarword,      "RSP!",       &rsp_at_word,     &rsp ) //@W
COM( rsshift_word,       rsshift_cb,     "2/",         &rsp_pling_word,  ) //@W
COM( source_word,        r2varword,      "SOURCE",     &rsshift_word,    input_buffer, (void*)MAX_LINE_LENGTH ) //@W
COM( sp0_word,           rvarword,       "SP0",        &source_word,     dstack+DSTACKSIZE ) //@W
COM( sp_at_word,         rivarword,      "SP@",        &sp0_word,        &dsp ) //@W
COM( sp_pling_word,      wivarword,      "SP!",        &sp_at_word,      &dsp ) //@W
COM( state_word,         rvarword,       "STATE",      &sp_pling_word,   &state ) //@W
COM( sub_one_word,       increment_cb,   "1-",         &state_word,      (void*)-1 ) //@W
COM( sub_word,           sub_cb,         "-",          &sub_one_word,    ) //@W
COM( swap_word,          swap_cb,        "SWAP",       &sub_word,        ) //@W
COM( t_drop_word,        t_drop_cb,      "2DROP",      &swap_word,       ) //@W
COM( t_dup_word,         peekcon2_cb,    "2DUP",       &t_drop_word,     (void*)1 ) //@W
COM( t_over_word,        peekcon2_cb,    "2OVER",      &t_dup_word,      (void*)3 ) //@W
COM( t_swap_word,        t_swap_cb,      "2SWAP",      &t_over_word,     ) //@W
COM( tuck_word,          tuck_cb,        "TUCK",       &t_swap_word,     ) //@W
COM( two_word,           rvarword,       "2",          &tuck_word,       (void*)2 ) //@W
COM( u_lt_word,          u_lt_cb,        "U<",         &two_word,        ) //@W
COM( u_m_star_word,      u_m_star_cb,    "UM*",        &u_lt_word,       ) //@W
COM( um_mod,             um_mod_cb,      "UM/MOD",     &u_m_star_word,   ) //@W
COM( word_word,          word_cb,        "WORD",       &um_mod,          ) //@W
COM( xor_word,           xor_cb,         "XOR",        &word_word,       ) //@W
COM( zero_word,          rvarword,       "0",          &xor_word,        (void*)0 ) //@W
IMM( immediate_word,     immediate_cb,   "IMMEDIATE",  &zero_word,       ) //@W
IMM( open_sq_word,       open_sq_cb,     "[",          &immediate_word,  ) //@W

//@C \ IMMEDIATE
//   10 WORD DROP
IMM( _5c__word, codeword, "\\", &open_sq_word, (void*)&lit_word, (void*)10, (void*)&word_word, (void*)&drop_word, (void*)&exit_word )

//@C CELLS
//  CELL *
COM( cells_word, codeword, "CELLS", &_5c__word, (void*)&cell_word, (void*)&mul_word, (void*)&exit_word )

//@C CELL+
//  CELL +
COM( cell_2b__word, codeword, "CELL+", &cells_word, (void*)&cell_word, (void*)&add_word, (void*)&exit_word )

//@C CELL-
//  CELL -
COM( cell_2d__word, codeword, "CELL-", &cell_2b__word, (void*)&cell_word, (void*)&sub_word, (void*)&exit_word )

//@C CHAR+
//  1+
COM( char_2b__word, codeword, "CHAR+", &cell_2d__word, (void*)&add_one_word, (void*)&exit_word )

//@C CHARS
// \ nop!
COM( chars_word, codeword, "CHARS", &char_2b__word, (void*)&exit_word )

//@C ALIGNED
// \ addr -- aligned-addr
//   DUP                               \ -- end-r-addr end-r-addr
//   CELL SWAP -
//   CELL 1- AND                       \ -- end-r-addr offset
//   +                                 \ -- aligned-end-r-addr
COM( aligned_word, codeword, "ALIGNED", &chars_word, (void*)&dup_word, (void*)&cell_word, (void*)&swap_word, (void*)&sub_word, (void*)&cell_word, (void*)&sub_one_word, (void*)&and_word, (void*)&add_word, (void*)&exit_word )

//@C +!
// \ n addr --
//   DUP @                             \ -- n addr val
//   ROT                               \ -- addr val n
//   +                                 \ -- addr new-val
//   SWAP !
COM( _2b__21__word, codeword, "+!", &aligned_word, (void*)&dup_word, (void*)&at_word, (void*)&rot_word, (void*)&add_word, (void*)&swap_word, (void*)&pling_word, (void*)&exit_word )

//@C ,
//  HERE !
//  CELL ALLOT
COM( _2c__word, codeword, ",", &_2b__21__word, (void*)&here_word, (void*)&pling_word, (void*)&cell_word, (void*)&allot_word, (void*)&exit_word )

//@C C,
//  HERE C!
//  1 ALLOT
COM( c_2c__word, codeword, "C,", &_2c__word, (void*)&here_word, (void*)&c_pling_word, (void*)&one_word, (void*)&allot_word, (void*)&exit_word )

//@C CREATE
//  \ Get the word name; this is written as a counted string to here.
//  32 WORD                            \ addr --
//
//  \ Advance over it.
//  DUP C@ 1+ ALLOT                    \ addr --
//
//  \ Ensure alignment, then create the low level header.
//  ALIGN [&_create_word]
COM( create_word, codeword, "CREATE", &c_2c__word, (void*)&lit_word, (void*)32, (void*)&word_word, (void*)&dup_word, (void*)&c_at_word, (void*)&add_one_word, (void*)&allot_word, (void*)&align_word, (void*)(&_create_word), (void*)&exit_word )

//@C EMIT
//   HERE C!
//   _stdout HERE 1 _write DROP
COM( emit_word, codeword, "EMIT", &create_word, (void*)&here_word, (void*)&c_pling_word, (void*)&_stdout_word, (void*)&here_word, (void*)&one_word, (void*)&_write_word, (void*)&drop_word, (void*)&exit_word )

//@C TYPE
// \ ( addr n -- )
//   _stdout ROT ROT _write DROP
COM( type_word, codeword, "TYPE", &emit_word, (void*)&_stdout_word, (void*)&rot_word, (void*)&rot_word, (void*)&_write_word, (void*)&drop_word, (void*)&exit_word )

//@C CR
//   10 EMIT
COM( cr_word, codeword, "CR", &type_word, (void*)&lit_word, (void*)10, (void*)&emit_word, (void*)&exit_word )

//@C BL
//   32
COM( bl_word, codeword, "BL", &cr_word, (void*)&lit_word, (void*)32, (void*)&exit_word )

//@C SPACE
//   BL EMIT
COM( space_word, codeword, "SPACE", &bl_word, (void*)&bl_word, (void*)&emit_word, (void*)&exit_word )

//@C SPACES
// \ n --
//   BEGIN
//     DUP 0>
//   WHILE
//     SPACE 1-
//   REPEAT
//   DROP
COM( spaces_word, codeword, "SPACES", &space_word, (void*)&dup_word, (void*)&more0_word, (void*)&branch0_word, (void*)(&spaces_word.payload[0] + 8), (void*)&space_word, (void*)&sub_one_word, (void*)&branch_word, (void*)(&spaces_word.payload[0] + 0), (void*)&drop_word, (void*)&exit_word )

//@C NEGATE
//   0 SWAP -
COM( negate_word, codeword, "NEGATE", &spaces_word, (void*)&zero_word, (void*)&swap_word, (void*)&sub_word, (void*)&exit_word )

//@C TRUE
//   1
COM( true_word, codeword, "TRUE", &negate_word, (void*)&one_word, (void*)&exit_word )

//@C FALSE
//   0
COM( false_word, codeword, "FALSE", &true_word, (void*)&zero_word, (void*)&exit_word )

//@C BYE
//   0 _exit
COM( bye_word, codeword, "BYE", &false_word, (void*)&zero_word, (void*)&_exit_word, (void*)&exit_word )

//@C REFILL
//  \ Read a line from the terminal.
//  SOURCE ACCEPT              \ -- len
//
//  \ Is this the end?
//  DUP 0< IF
//    DROP 0 EXIT
//  THEN
//
//  \ Clear the remainder of the buffer.
//  DUP [&lit_word] [input_buffer] +       \ -- len addr
//  SWAP                                   \ -- addr len
//  [&lit_word] [MAX_LINE_LENGTH] SWAP -   \ -- addr remaining
//  32                                     \ -- addr remaining char
//  FILL
//
//  \ Reset the input pointer.
//  0 >IN !
//
//  \ We must succeed!
//  1
COM( refill_word, codeword, "REFILL", &bye_word, (void*)&source_word, (void*)&accept_word, (void*)&dup_word, (void*)&less0_word, (void*)&branch0_word, (void*)(&refill_word.payload[0] + 9), (void*)&drop_word, (void*)&zero_word, (void*)&exit_word, (void*)&dup_word, (void*)(&lit_word), (void*)(input_buffer), (void*)&add_word, (void*)&swap_word, (void*)(&lit_word), (void*)(MAX_LINE_LENGTH), (void*)&swap_word, (void*)&sub_word, (void*)&lit_word, (void*)32, (void*)&fill_word, (void*)&zero_word, (void*)&in_arrow_word, (void*)&pling_word, (void*)&one_word, (void*)&exit_word )

//@C COUNT
// \ ( c-addr -- addr len )
//   DUP C@ SWAP 1+ SWAP
COM( count_word, codeword, "COUNT", &refill_word, (void*)&dup_word, (void*)&c_at_word, (void*)&swap_word, (void*)&add_one_word, (void*)&swap_word, (void*)&exit_word )

//@C ( IMMEDIATE
//   41 WORD DROP
IMM( _28__word, codeword, "(", &count_word, (void*)&lit_word, (void*)41, (void*)&word_word, (void*)&drop_word, (void*)&exit_word )

//@C numberthenneg HIDDEN
// \ val addr len -- val addr len
// \ As >NUMBER, but negates the result.
//   >NUMBER
//   ROT NEGATE ROT ROT
COM( numberthenneg_word, codeword, "", &_28__word, (void*)&a_number_word, (void*)&rot_word, (void*)&negate_word, (void*)&rot_word, (void*)&rot_word, (void*)&exit_word )

//@C snumber HIDDEN
// \ val addr len -- val addr len
// \ As >NUMBER, but copes with a leading -.
//
//   SWAP DUP C@                       \ -- val len addr byte
//   ROT SWAP                          \ -- val addr len byte
//   45 = IF
//     1- SWAP 1+ SWAP
//     numberthenneg
//   ELSE
//     >NUMBER
//   THEN
COM( snumber_word, codeword, "", &numberthenneg_word, (void*)&swap_word, (void*)&dup_word, (void*)&c_at_word, (void*)&rot_word, (void*)&swap_word, (void*)&lit_word, (void*)45, (void*)&equals_word, (void*)&branch0_word, (void*)(&snumber_word.payload[0] + 17), (void*)&sub_one_word, (void*)&swap_word, (void*)&add_one_word, (void*)&swap_word, (void*)&numberthenneg_word, (void*)&branch_word, (void*)(&snumber_word.payload[0] + 18), (void*)&a_number_word, (void*)&exit_word )

//@C LITERAL IMMEDIATE
//   [&lit_word] [&lit_word] , ,
IMM( literal_word, codeword, "LITERAL", &snumber_word, (void*)(&lit_word), (void*)(&lit_word), (void*)&_2c__word, (void*)&_2c__word, (void*)&exit_word )

CONST_STRING(unrecognised_word_msg, "panic: unrecognised word: ");
//@C E_enoent HIDDEN
// \ c-addr --
//   [&lit_word] [unrecognised_word_msg]
//   [&lit_word] [sizeof(unrecognised_word_msg)]
//   TYPE
//
//   DROP DROP                         \ -- addr
//   COUNT                             \ -- c-addr len
//   TYPE                              \ --
//
//   CR
//   ABORT
COM( e_enoent_word, codeword, "", &literal_word, (void*)(&lit_word), (void*)(unrecognised_word_msg), (void*)(&lit_word), (void*)(sizeof(unrecognised_word_msg)), (void*)&type_word, (void*)&drop_word, (void*)&drop_word, (void*)&count_word, (void*)&type_word, (void*)&cr_word, (void*)&abort_word, (void*)&exit_word )

CONST_STRING(end_of_line_msg, "panic: unexpected end of line");
//@C E_eol HIDDEN
// \ --
//   [&lit_word] [unrecognised_word_msg]
//   [&lit_word] [sizeof(unrecognised_word_msg)]
//   TYPE
//   CR
//   ABORT
COM( e_eol_word, codeword, "", &e_enoent_word, (void*)(&lit_word), (void*)(unrecognised_word_msg), (void*)(&lit_word), (void*)(sizeof(unrecognised_word_msg)), (void*)&type_word, (void*)&cr_word, (void*)&abort_word, (void*)&exit_word )

//@C INTERPRET_NUM HIDDEN
// \ Evaluates a number, or perish in the attempt.
// \ ( c-addr -- value )
//   DUP
//
//   \ Get the length of the input string.
//   DUP C@                            \ -- addr addr len
//
//   \ The address we've got is a counted string; we want the address of the data.
//   SWAP 1+                           \ -- addr len addr+1
//
//   \ Initialise the accumulator.
//   0 SWAP ROT                        \ -- addr 0 addr+1 len
//
//   \ Parse!
//   snumber                           \ -- addr val addr+1 len
//
//   \ We must consume all bytes to succeed.
//   IF E_enoent THEN
//
//   \ Huzzah!                         \ -- addr val addr+1
//   DROP SWAP DROP                    \ -- val
COM( interpret_num_word, codeword, "", &e_eol_word, (void*)&dup_word, (void*)&dup_word, (void*)&c_at_word, (void*)&swap_word, (void*)&add_one_word, (void*)&zero_word, (void*)&swap_word, (void*)&rot_word, (void*)&snumber_word, (void*)&branch0_word, (void*)(&interpret_num_word.payload[0] + 12), (void*)&e_enoent_word, (void*)&drop_word, (void*)&swap_word, (void*)&drop_word, (void*)&exit_word )

//@C COMPILE_NUM HIDDEN
// \ Compiles a number (or at least, a word we don't recognise).
// \ ( c-addr -- )
//   \ The interpreter does the heavy lifting for us!
//   INTERPRET_NUM LITERAL
COM( compile_num_word, codeword, "", &interpret_num_word, (void*)&interpret_num_word, (void*)&literal_word, (void*)&exit_word )

static cdefn_t* interpreter_table[] =
{
	// compiling   not found            immediate
	&execute_word, &interpret_num_word, &execute_word, // interpreting
	&_2c__word,    &compile_num_word,   &execute_word  // compiling
};

//@C INTERPRET
// \ Parses the input buffer and executes the words therein.
//   BEGIN
//     \ Parse a word.
//     \ (This relies of word writing the result to here.)
//     32 WORD                         \ -- c-addr
//
//     \ End of the buffer? If so, return.
//     C@ 0= IF EXIT THEN              \ --
//
//     \ Look up the word.
//     HERE FIND                     \ -- addr kind
//
//     \ What is it? Calculate an offset into the lookup table.
//     1+ CELLS
//     STATE @ 24 *
//     +                               \ -- addr offset
//
//     \ Look up the right word and run it.
//     [&lit_word] [interpreter_table] + @ EXECUTE \ -- addr
//   AGAIN
COM( interpret_word, codeword, "INTERPRET", &compile_num_word, (void*)&lit_word, (void*)32, (void*)&word_word, (void*)&c_at_word, (void*)&equals0_word, (void*)&branch0_word, (void*)(&interpret_word.payload[0] + 8), (void*)&exit_word, (void*)&here_word, (void*)&find_word, (void*)&add_one_word, (void*)&cells_word, (void*)&state_word, (void*)&at_word, (void*)&lit_word, (void*)24, (void*)&mul_word, (void*)&add_word, (void*)(&lit_word), (void*)(interpreter_table), (void*)&add_word, (void*)&at_word, (void*)&execute_word, (void*)&branch_word, (void*)(&interpret_word.payload[0] + 0), (void*)&exit_word )

static const char prompt_msg[4] = " ok\n";
//@C INTERACT
//  BEGIN
//    \ If we're reading from stdin, show the prompt.
//    _input_fd @ _stdin = IF
//      [&lit_word] [prompt_msg] 4 TYPE
//    THEN
//
//    \ Refill the input buffer; if we run out, exit.
//    REFILL 0= IF EXIT THEN
//
//    \ Interpret the contents of the buffer.
//    INTERPRET
//  AGAIN
COM( interact_word, codeword, "INTERACT", &interpret_word, (void*)&_input_fd_word, (void*)&at_word, (void*)&_stdin_word, (void*)&equals_word, (void*)&branch0_word, (void*)(&interact_word.payload[0] + 11), (void*)(&lit_word), (void*)(prompt_msg), (void*)&lit_word, (void*)4, (void*)&type_word, (void*)&refill_word, (void*)&equals0_word, (void*)&branch0_word, (void*)(&interact_word.payload[0] + 16), (void*)&exit_word, (void*)&interpret_word, (void*)&branch_word, (void*)(&interact_word.payload[0] + 0), (void*)&exit_word )

//@C QUIT
//  SP0 SP!
//  RSP0 RSP!
//  0 STATE !
//  INTERACT BYE
COM( quit_word, codeword, "QUIT", &interact_word, (void*)&sp0_word, (void*)&sp_pling_word, (void*)&rsp0_word, (void*)&rsp_pling_word, (void*)&zero_word, (void*)&state_word, (void*)&pling_word, (void*)&interact_word, (void*)&bye_word, (void*)&exit_word )

//@C READ-FILE
//   \ Read the filename.
//   32 WORD
//
//   \ Turn it into a C string.
//   DUP C@ + 1+ 0 SWAP C!
//
//   \ Open the new file.
//   HERE 1+ O_RDONLY _open
//   DUP 0= IF E_fnf THEN
//
//   \ Swap in the new stream, saving the old one to the stack.
//   _input_fd @
//   SWAP _input_fd !
//
//   \ Run the interpreter/compiler until EOF.
//   INTERACT
//
//   \ Close the new stream.
//   _input_fd @ _close DROP
//
//   \ Restore the old stream.
//   _input_fd !
COM( read_2d_file_word, codeword, "READ-FILE", &quit_word, (void*)&lit_word, (void*)32, (void*)&word_word, (void*)&dup_word, (void*)&c_at_word, (void*)&add_word, (void*)&add_one_word, (void*)&zero_word, (void*)&swap_word, (void*)&c_pling_word, (void*)&here_word, (void*)&add_one_word, (void*)&_O_RDONLY_word, (void*)&_open_word, (void*)&dup_word, (void*)&equals0_word, (void*)&branch0_word, (void*)(&read_2d_file_word.payload[0] + 19), (void*)&E_fnf_word, (void*)&_input_fd_word, (void*)&at_word, (void*)&swap_word, (void*)&_input_fd_word, (void*)&pling_word, (void*)&interact_word, (void*)&_input_fd_word, (void*)&at_word, (void*)&_close_word, (void*)&drop_word, (void*)&_input_fd_word, (void*)&pling_word, (void*)&exit_word )

//@C :
//  \ Create the word itself.
//  CREATE
//
//  \ Turn it into a runnable word.
//  [&lit_word] [codeword] LATEST @ !
//
//  \ Switch to compilation mode.
//  ]
COM( _3a__word, codeword, ":", &read_2d_file_word, (void*)&create_word, (void*)(&lit_word), (void*)(codeword), (void*)&latest_word, (void*)&at_word, (void*)&pling_word, (void*)&close_sq_word, (void*)&exit_word )

//@C ; IMMEDIATE
//  [&lit_word] [&exit_word] ,
//  [
IMM( _3b__word, codeword, ";", &_3a__word, (void*)(&lit_word), (void*)(&exit_word), (void*)&_2c__word, (void*)&open_sq_word, (void*)&exit_word )

//@C CONSTANT
// \ ( value -- )
//  CREATE
//  [&lit_word] [rvarword] LATEST @ !
//  ,
COM( constant_word, codeword, "CONSTANT", &_3b__word, (void*)&create_word, (void*)(&lit_word), (void*)(rvarword), (void*)&latest_word, (void*)&at_word, (void*)&pling_word, (void*)&_2c__word, (void*)&exit_word )

//@C VARIABLE
//  CREATE 0 ,
COM( variable_word, codeword, "VARIABLE", &constant_word, (void*)&create_word, (void*)&zero_word, (void*)&_2c__word, (void*)&exit_word )

//@C 'andkind HIDDEN
// \ Picks up a word from the input stream and returns its xt and type.
// \ Aborts on error.
// \ -- xt kind
//   32 WORD                           \ -- c-addr
//
//   \ End of the buffer? If so, panic..
//   C@ 0= IF E_eol THEN               \ --
//
//   \ Look up the word.
//   HERE FIND                       \ -- addr kind
//
//   \ Not found?
//   DUP 0= IF E_enoent THEN               \ -- addr
//
COM( _27_andkind_word, codeword, "", &variable_word, (void*)&lit_word, (void*)32, (void*)&word_word, (void*)&c_at_word, (void*)&equals0_word, (void*)&branch0_word, (void*)(&_27_andkind_word.payload[0] + 8), (void*)&e_eol_word, (void*)&here_word, (void*)&find_word, (void*)&dup_word, (void*)&equals0_word, (void*)&branch0_word, (void*)(&_27_andkind_word.payload[0] + 15), (void*)&e_enoent_word, (void*)&exit_word )

//@C '
//   'andkind DROP
COM( _27__word, codeword, "'", &_27_andkind_word, (void*)&_27_andkind_word, (void*)&drop_word, (void*)&exit_word )

//@C ['] IMMEDIATE
//   ' LITERAL
IMM( _5b__27__5d__word, codeword, "[']", &_27__word, (void*)&_27__word, (void*)&literal_word, (void*)&exit_word )

//@C POSTPONE IMMEDIATE
//   'andkind                          \ -- xt kind
//   -1 = IF                           \ -- xt
//     \ Normal word --- generate code to compile it.
//     LITERAL [&lit_word] [&_2c__word] ,
//   ELSE
//     \ Immediate word --- generate code to run it.
//     ,
//   THEN
IMM( postpone_word, codeword, "POSTPONE", &_5b__27__5d__word, (void*)&_27_andkind_word, (void*)&m_one_word, (void*)&equals_word, (void*)&branch0_word, (void*)(&postpone_word.payload[0] + 11), (void*)&literal_word, (void*)(&lit_word), (void*)(&_2c__word), (void*)&_2c__word, (void*)&branch_word, (void*)(&postpone_word.payload[0] + 12), (void*)&_2c__word, (void*)&exit_word )

//@C IF IMMEDIATE
// \ -- addr
//  [&lit_word] [&branch0_word] ,
//  HERE
//  0 ,
IMM( if_word, codeword, "IF", &postpone_word, (void*)(&lit_word), (void*)(&branch0_word), (void*)&_2c__word, (void*)&here_word, (void*)&zero_word, (void*)&_2c__word, (void*)&exit_word )

//@C THEN IMMEDIATE
// \ addr --
//  HERE SWAP !
IMM( then_word, codeword, "THEN", &if_word, (void*)&here_word, (void*)&swap_word, (void*)&pling_word, (void*)&exit_word )

//@C ELSE IMMEDIATE
// \ if-addr -- else-addr
//  \ Emit a branch over the false part.
//  [&lit_word] [&branch_word] ,      \ -- if-addr
//
//  \ Remember where the branch label is for patching later.
//  HERE 0 ,                         \ -- if-addr else-addr
//
//  \ Patch the *old* branch label (from the condition) to the current address.
//  SWAP                               \ -- else-addr if-addr
//  [&then_word]
IMM( else_word, codeword, "ELSE", &then_word, (void*)(&lit_word), (void*)(&branch_word), (void*)&_2c__word, (void*)&here_word, (void*)&zero_word, (void*)&_2c__word, (void*)&swap_word, (void*)(&then_word), (void*)&exit_word )

//@C BEGIN IMMEDIATE
// \ -- start-addr
//  HERE
IMM( begin_word, codeword, "BEGIN", &else_word, (void*)&here_word, (void*)&exit_word )

//@C AGAIN IMMEDIATE
// \ start-addr --
//   [&lit_word] [&branch_word] , ,
IMM( again_word, codeword, "AGAIN", &begin_word, (void*)(&lit_word), (void*)(&branch_word), (void*)&_2c__word, (void*)&_2c__word, (void*)&exit_word )

//@C UNTIL IMMEDIATE
// \ start-addr --
//   [&lit_word] [&branch0_word] , ,
IMM( until_word, codeword, "UNTIL", &again_word, (void*)(&lit_word), (void*)(&branch0_word), (void*)&_2c__word, (void*)&_2c__word, (void*)&exit_word )

//@C WHILE IMMEDIATE
// \ Used as 'begin <cond> while <loop-body> repeat'.
// \ start-addr -- while-target-addr start-addr
//   [&lit_word] [&branch0_word] ,
//   HERE
//   0 ,
//   SWAP
IMM( while_word, codeword, "WHILE", &until_word, (void*)(&lit_word), (void*)(&branch0_word), (void*)&_2c__word, (void*)&here_word, (void*)&zero_word, (void*)&_2c__word, (void*)&swap_word, (void*)&exit_word )

//@C REPEAT IMMEDIATE
// \ while-target-addr start-addr --
//   [&lit_word] [&branch_word] , ,
//
//   HERE SWAP !
IMM( repeat_word, codeword, "REPEAT", &while_word, (void*)(&lit_word), (void*)(&branch_word), (void*)&_2c__word, (void*)&_2c__word, (void*)&here_word, (void*)&swap_word, (void*)&pling_word, (void*)&exit_word )

//@C DO IMMEDIATE
// \ C: -- &leave-addr start-addr
// \ R: -- leave-addr index max
// \    max index --
//   \ Save the loop exit address; this will be patched by LOOP.
//   [&lit_word] [&lit_word] ,
//   HERE 0 ,
//   [&lit_word] [&arrow_r_word] ,
//
//   \ Save loop start address onto the compiler's stack.
//   HERE
//
//   \ Push the index and max values onto the return stack.
//   [&lit_word] [&arrow_r_word] ,
//   [&lit_word] [&arrow_r_word] ,
IMM( do_word, codeword, "DO", &repeat_word, (void*)(&lit_word), (void*)(&lit_word), (void*)&_2c__word, (void*)&here_word, (void*)&zero_word, (void*)&_2c__word, (void*)(&lit_word), (void*)(&arrow_r_word), (void*)&_2c__word, (void*)&here_word, (void*)(&lit_word), (void*)(&arrow_r_word), (void*)&_2c__word, (void*)(&lit_word), (void*)(&arrow_r_word), (void*)&_2c__word, (void*)&exit_word )

//@C loophelper HIDDEN
// \ Contains the actual logic for loop.
// \ R: leave-addr index max r-addr -- leave-addr r-addr
// \    incr -- max index flag
//   \ Fetch data from return stack.
//   R> SWAP R> R> ROT                 \ r-addr max index incr  R: leave-addr
//   DUP ROT                           \ r-addr max incr incr index
//   + SWAP                            \ r-addr max index' incr 
//   0< IF
//     \ Counting down!
//     2DUP >                          \ r-addr max index' flag
//   ELSE
//     \ Counting up.
//     2DUP <=                         \ r-addr max index' flag
//   THEN
//     
//   \ Put the return address back!
//   >R                                \ r-addr max index'
//   ROT                               \ max index' r-addr
//   R>                                \ max index' r-addr flag
//   SWAP                              \ max index' flag r-addr
//   >R                                \ max index' flag
COM( loophelper_word, codeword, "", &do_word, (void*)&r_arrow_word, (void*)&swap_word, (void*)&r_arrow_word, (void*)&r_arrow_word, (void*)&rot_word, (void*)&dup_word, (void*)&rot_word, (void*)&add_word, (void*)&swap_word, (void*)&less0_word, (void*)&branch0_word, (void*)(&loophelper_word.payload[0] + 16), (void*)&t_dup_word, (void*)&gt_word, (void*)&branch_word, (void*)(&loophelper_word.payload[0] + 18), (void*)&t_dup_word, (void*)&le_word, (void*)&arrow_r_word, (void*)&rot_word, (void*)&r_arrow_word, (void*)&swap_word, (void*)&arrow_r_word, (void*)&exit_word )

//@C +LOOP IMMEDIATE
// \    incr --
// \ R: leave-addr index max --
// \ C: &leave-addr start-addr --
//   [&lit_word] [&loophelper_word] ,
//   [&lit_word] [&branch0_word] , ,
//   [&lit_word] [&t_drop_word] ,
//   [&lit_word] [&rdrop_word] ,
//
//   \ Patch the leave address to contain the loop exit address.
//   HERE SWAP !
IMM( _2b_loop_word, codeword, "+LOOP", &loophelper_word, (void*)(&lit_word), (void*)(&loophelper_word), (void*)&_2c__word, (void*)(&lit_word), (void*)(&branch0_word), (void*)&_2c__word, (void*)&_2c__word, (void*)(&lit_word), (void*)(&t_drop_word), (void*)&_2c__word, (void*)(&lit_word), (void*)(&rdrop_word), (void*)&_2c__word, (void*)&here_word, (void*)&swap_word, (void*)&pling_word, (void*)&exit_word )

//@C LOOP IMMEDIATE
//   1 LITERAL +LOOP
IMM( loop_word, codeword, "LOOP", &_2b_loop_word, (void*)&one_word, (void*)&literal_word, (void*)&_2b_loop_word, (void*)&exit_word )

//@C LEAVE
// \ R: leave-addr index max
//   \ Remove LEAVE's return address.
//   RDROP
//
//   \ ...and the two control words.
//   RDROP RDROP
//
//   \ All that's left is the loop exit address, and EXIT
//   \ will consume that.
COM( leave_word, codeword, "LEAVE", &loop_word, (void*)&rdrop_word, (void*)&rdrop_word, (void*)&rdrop_word, (void*)&exit_word )

//@C UNLOOP
// \ R: leave-addr index max
//   R> RDROP RDROP RDROP >R
COM( unloop_word, codeword, "UNLOOP", &leave_word, (void*)&r_arrow_word, (void*)&rdrop_word, (void*)&rdrop_word, (void*)&rdrop_word, (void*)&arrow_r_word, (void*)&exit_word )

//@C I
// \ R: leave-addr index max -- leave-addr index max
// \    -- index
//  2 RPICK
COM( i_word, codeword, "I", &unloop_word, (void*)&two_word, (void*)&rpick_word, (void*)&exit_word )

//@C J
// \ R: leave-addr index max -- leave-addr index max
// \    -- index
//  5 RPICK
COM( j_word, codeword, "J", &i_word, (void*)&lit_word, (void*)5, (void*)&rpick_word, (void*)&exit_word )

//@C HEX
//  16 BASE !
COM( hex_word, codeword, "HEX", &j_word, (void*)&lit_word, (void*)16, (void*)&base_word, (void*)&pling_word, (void*)&exit_word )

//@C DECIMAL
//  10 BASE !
COM( decimal_word, codeword, "DECIMAL", &hex_word, (void*)&lit_word, (void*)10, (void*)&base_word, (void*)&pling_word, (void*)&exit_word )

//@C S>D
//   DUP 0< IF -1 ELSE 0 THEN 
COM( s_3e_d_word, codeword, "S>D", &decimal_word, (void*)&dup_word, (void*)&less0_word, (void*)&branch0_word, (void*)(&s_3e_d_word.payload[0] + 7), (void*)&m_one_word, (void*)&branch_word, (void*)(&s_3e_d_word.payload[0] + 8), (void*)&zero_word, (void*)&exit_word )

//@C R@
//   1 RPICK
COM( r_40__word, codeword, "R@", &s_3e_d_word, (void*)&one_word, (void*)&rpick_word, (void*)&exit_word )

//@C +- HIDDEN
//   0< IF NEGATE THEN
COM( _2b__2d__word, codeword, "", &r_40__word, (void*)&less0_word, (void*)&branch0_word, (void*)(&_2b__2d__word.payload[0] + 4), (void*)&negate_word, (void*)&exit_word )

//@C SM/REM
//   OVER                              \ low high quot high
//   >R >R
//   DABS R@ ABS
//   UM/MOD
//   R> R@ XOR
//   +-
//   SWAP R>
//   +-
//   SWAP
COM( sm_2f_rem_word, codeword, "SM/REM", &_2b__2d__word, (void*)&over_word, (void*)&arrow_r_word, (void*)&arrow_r_word, (void*)&dabs_word, (void*)&r_40__word, (void*)&abs_word, (void*)&um_mod, (void*)&r_arrow_word, (void*)&r_40__word, (void*)&xor_word, (void*)&_2b__2d__word, (void*)&swap_word, (void*)&r_arrow_word, (void*)&_2b__2d__word, (void*)&swap_word, (void*)&exit_word )

//@C /
//  >R S>D R> FM/MOD SWAP DROP
COM( _2f__word, codeword, "/", &sm_2f_rem_word, (void*)&arrow_r_word, (void*)&s_3e_d_word, (void*)&r_arrow_word, (void*)&fm_mod_word, (void*)&swap_word, (void*)&drop_word, (void*)&exit_word )

//@C /MOD
//  >R S>D R> FM/MOD
COM( _2f_mod_word, codeword, "/MOD", &_2f__word, (void*)&arrow_r_word, (void*)&s_3e_d_word, (void*)&r_arrow_word, (void*)&fm_mod_word, (void*)&exit_word )

//@C MOD
//  >R S>D R> FM/MOD DROP
COM( mod_word, codeword, "MOD", &_2f_mod_word, (void*)&arrow_r_word, (void*)&s_3e_d_word, (void*)&r_arrow_word, (void*)&fm_mod_word, (void*)&drop_word, (void*)&exit_word )

//@C */
//  >R M* R> FM/MOD SWAP DROP
COM( _2a__2f__word, codeword, "*/", &mod_word, (void*)&arrow_r_word, (void*)&m_star_word, (void*)&r_arrow_word, (void*)&fm_mod_word, (void*)&swap_word, (void*)&drop_word, (void*)&exit_word )

//@C */MOD
//  >R M* R> FM/MOD
COM( _2a__2f_mod_word, codeword, "*/MOD", &_2a__2f__word, (void*)&arrow_r_word, (void*)&m_star_word, (void*)&r_arrow_word, (void*)&fm_mod_word, (void*)&exit_word )

//@C DEPTH
//  SP0 SP@ - CELL / 1-
COM( depth_word, codeword, "DEPTH", &_2a__2f_mod_word, (void*)&sp0_word, (void*)&sp_at_word, (void*)&sub_word, (void*)&cell_word, (void*)&_2f__word, (void*)&sub_one_word, (void*)&exit_word )

//@C s"helper HIDDEN
// \ -- addr count
//  \ The return address points at a counted string.
//  R> DUP                             \ -- r-addr r-addr
//
//  \ Move it to point after the string.
//  DUP C@ + 1+                        \ -- r-addr end-r-addr
//
//  \ Align it!
//  ALIGNED
//
//  \ Store it back as the return address.
//  >R
//
//  \ ...and decode the counted string.
//  COUNT
COM( s_22_helper_word, codeword, "", &depth_word, (void*)&r_arrow_word, (void*)&dup_word, (void*)&dup_word, (void*)&c_at_word, (void*)&add_word, (void*)&add_one_word, (void*)&aligned_word, (void*)&arrow_r_word, (void*)&count_word, (void*)&exit_word )

//@C S" IMMEDIATE
// \ -- addr count
//  \ Emit the helper.
//  [&lit_word] [&s_22_helper_word] ,
//
//  \ Emit the text itself as a counted string.
//  34 WORD
//
//  \ Advance over the text.
//  C@ 1+ ALLOT
//
//  \ Make sure the workspace pointer is aligned!
//  ALIGN
IMM( s_22__word, codeword, "S\"", &s_22_helper_word, (void*)(&lit_word), (void*)(&s_22_helper_word), (void*)&_2c__word, (void*)&lit_word, (void*)34, (void*)&word_word, (void*)&c_at_word, (void*)&add_one_word, (void*)&allot_word, (void*)&align_word, (void*)&exit_word )

//@C 2*
//  1 LSHIFT
COM( _32__2a__word, codeword, "2*", &s_22__word, (void*)&one_word, (void*)&lshift_word, (void*)&exit_word )

//@C MIN
// \ x1 x2 -- x3
//   2DUP > IF SWAP THEN DROP
COM( min_word, codeword, "MIN", &_32__2a__word, (void*)&t_dup_word, (void*)&gt_word, (void*)&branch0_word, (void*)(&min_word.payload[0] + 5), (void*)&swap_word, (void*)&drop_word, (void*)&exit_word )

//@C MAX
// \ x1 x2 -- x3
//   2DUP < IF SWAP THEN DROP
COM( max_word, codeword, "MAX", &min_word, (void*)&t_dup_word, (void*)&lt_word, (void*)&branch0_word, (void*)(&max_word.payload[0] + 5), (void*)&swap_word, (void*)&drop_word, (void*)&exit_word )

//@C RECURSE IMMEDIATE
//   LATEST @ ,
IMM( recurse_word, codeword, "RECURSE", &max_word, (void*)&latest_word, (void*)&at_word, (void*)&_2c__word, (void*)&exit_word )

//@C u.nospace HIDDEN
// \ u --
//   BASE @ /MOD
//   ?DUP IF
//     RECURSE
//   THEN
//
//   DUP 10 < IF
//     48
//   ELSE
//     10 -
//     65
//   THEN
//   + EMIT
COM( u_2e_nospace_word, codeword, "", &recurse_word, (void*)&base_word, (void*)&at_word, (void*)&_2f_mod_word, (void*)&q_dup_word, (void*)&branch0_word, (void*)(&u_2e_nospace_word.payload[0] + 7), (void*)&u_2e_nospace_word, (void*)&dup_word, (void*)&lit_word, (void*)10, (void*)&lt_word, (void*)&branch0_word, (void*)(&u_2e_nospace_word.payload[0] + 17), (void*)&lit_word, (void*)48, (void*)&branch_word, (void*)(&u_2e_nospace_word.payload[0] + 22), (void*)&lit_word, (void*)10, (void*)&sub_word, (void*)&lit_word, (void*)65, (void*)&add_word, (void*)&emit_word, (void*)&exit_word )

//@C uwidth HIDEEN
// \ This word returns the width (in characters) of an unsigned number in the current base.
// \ u -- width
//   BASE @ /
//   ?DUP IF
//     RECURSE 1+
//   ELSE
//     1
//   THEN
COM( uwidth_word, codeword, "uwidth", &u_2e_nospace_word, (void*)&base_word, (void*)&at_word, (void*)&_2f__word, (void*)&q_dup_word, (void*)&branch0_word, (void*)(&uwidth_word.payload[0] + 10), (void*)&uwidth_word, (void*)&add_one_word, (void*)&branch_word, (void*)(&uwidth_word.payload[0] + 11), (void*)&one_word, (void*)&exit_word )

//@C U.R
// \ Prints an unsigned number in a field.
// \ u width --
//   SWAP DUP                          \ width u u
//   uwidth                            \ width u uwidth
//   ROT                               \ u uwidth width
//   SWAP -                            \ u width-uwidth
//
//   SPACES u.nospace
COM( u_2e_r_word, codeword, "U.R", &uwidth_word, (void*)&swap_word, (void*)&dup_word, (void*)&uwidth_word, (void*)&rot_word, (void*)&swap_word, (void*)&sub_word, (void*)&spaces_word, (void*)&u_2e_nospace_word, (void*)&exit_word )

//@C .R
// \ Prints a signed number in a field. We can't just print the sign and call
// \ U.R, because we want the sign to be next to the number...
// \ n width --
//
//   SWAP                              \ width n
//   DUP 0< IF
//     NEGATE                          \ width u
//     1
//     SWAP ROT 1-                     \ 1 u width-1
//   ELSE
//     0
//     SWAP ROT                        \ 0 u width
//   THEN
//
//   SWAP DUP                          \ flag width u u
//   uwidth                            \ flag width u uwidth
//   ROT                               \ flag u uwidth width
//   SWAP -                            \ flag u width-uwidth
//
//   SPACES                            \ flag u
//   SWAP                              \ u flag
//
//   IF 45 EMIT THEN
//
//   u.nospace
COM( _2e_r_word, codeword, ".R", &u_2e_r_word, (void*)&swap_word, (void*)&dup_word, (void*)&less0_word, (void*)&branch0_word, (void*)(&_2e_r_word.payload[0] + 12), (void*)&negate_word, (void*)&one_word, (void*)&swap_word, (void*)&rot_word, (void*)&sub_one_word, (void*)&branch_word, (void*)(&_2e_r_word.payload[0] + 15), (void*)&zero_word, (void*)&swap_word, (void*)&rot_word, (void*)&swap_word, (void*)&dup_word, (void*)&uwidth_word, (void*)&rot_word, (void*)&swap_word, (void*)&sub_word, (void*)&spaces_word, (void*)&swap_word, (void*)&branch0_word, (void*)(&_2e_r_word.payload[0] + 28), (void*)&lit_word, (void*)45, (void*)&emit_word, (void*)&u_2e_nospace_word, (void*)&exit_word )

//@C .
//   0 .R SPACE
COM( _2e__word, codeword, ".", &_2e_r_word, (void*)&zero_word, (void*)&_2e_r_word, (void*)&space_word, (void*)&exit_word )

//@C U.
//   0 U.R SPACE
COM( u_2e__word, codeword, "U.", &_2e__word, (void*)&zero_word, (void*)&u_2e_r_word, (void*)&space_word, (void*)&exit_word )

//@C showstack HIDDEN
// \ Dumps the contents of a stack.
// \ ( SP@ SP0 -- )
//   BEGIN
//     2DUP <>
//   WHILE
//     CELL-
//     DUP @ .
//   REPEAT
//   2DROP
//   CR
COM( showstack_word, codeword, "", &u_2e__word, (void*)&t_dup_word, (void*)&not_equals_word, (void*)&branch0_word, (void*)(&showstack_word.payload[0] + 10), (void*)&cell_2d__word, (void*)&dup_word, (void*)&at_word, (void*)&_2e__word, (void*)&branch_word, (void*)(&showstack_word.payload[0] + 0), (void*)&t_drop_word, (void*)&cr_word, (void*)&exit_word )

//@C .S
//   SP@ SP0 showstack
COM( _2e_s_word, codeword, ".S", &showstack_word, (void*)&sp_at_word, (void*)&sp0_word, (void*)&showstack_word, (void*)&exit_word )

//@C .RS
//   RSP@ CELL+ RSP0 showstack
COM( _2e_rs_word, codeword, ".RS", &_2e_s_word, (void*)&rsp_at_word, (void*)&cell_2b__word, (void*)&rsp0_word, (void*)&showstack_word, (void*)&exit_word )

//@C CHAR
//   BL WORD
//   DUP C@ 0= IF E_eol THEN
//   1+ C@
COM( char_word, codeword, "CHAR", &_2e_rs_word, (void*)&bl_word, (void*)&word_word, (void*)&dup_word, (void*)&c_at_word, (void*)&equals0_word, (void*)&branch0_word, (void*)(&char_word.payload[0] + 8), (void*)&e_eol_word, (void*)&add_one_word, (void*)&c_at_word, (void*)&exit_word )

//@C [CHAR] IMMEDIATE
//   CHAR LITERAL
IMM( _5b_char_5d__word, codeword, "[CHAR]", &char_word, (void*)&char_word, (void*)&literal_word, (void*)&exit_word )

//@C ." IMMEDIATE
//   \ Load string literal.
//   S\"
//
//   \ Print it.
//   [&lit_word] [&type_word] ,
IMM( _2e__22__word, codeword, ".\"", &_5b_char_5d__word, (void*)&s_22__word, (void*)(&lit_word), (void*)(&type_word), (void*)&_2c__word, (void*)&exit_word )

//@C 2@
//   DUP CELL+ @ SWAP @
COM( _32__40__word, codeword, "2@", &_2e__22__word, (void*)&dup_word, (void*)&cell_2b__word, (void*)&at_word, (void*)&swap_word, (void*)&at_word, (void*)&exit_word )

//@C 2!
//   SWAP OVER ! CELL+ !
COM( _32__21__word, codeword, "2!", &_32__40__word, (void*)&swap_word, (void*)&over_word, (void*)&pling_word, (void*)&cell_2b__word, (void*)&pling_word, (void*)&exit_word )

static cdefn_t* last = (defn_t*) &_32__21__word; //@E
static defn_t* latest = (defn_t*) &_32__21__word; //@E

int main(int argc, const char* argv[])
{
	here = here_top = sbrk(0);
	claim_workspace(0);

	setjmp(onerror);
	input_fd = 0;

	if (argc > 1)
	{
		input_fd = open(argv[1], O_RDONLY);

		/* Panics when running a script exit, rather than returning to the
		 * REPL. */
		if (setjmp(onerror))
			exit(1);

		if (input_fd == -1)
		{
			strerr("panic: unable to open file: ");
			strerr(argv[1]);
			strerr("\n");
			exit(1);
		}
	}
			
	dsp = dstack + DSTACKSIZE;
	rsp = rstack + RSTACKSIZE;

	pc = (defn_t**) &quit_word.payload[0];
	for (;;)
	{
		const struct definition* w = (void*) *pc++;
		/* Uncomment this to trace the current program counter, stack and
		 * word for every bytecode. */
		#if 0
			cell_t* p;
			printf("%p ", pc-1);
			printf("S(");
			for (p = dstack+DSTACKSIZE-1; p >= dsp; p--)
				printf("%lx ", *p);
			printf(") ");
			/* Uncomment this to also trace the return stack. */
			#if 1
				printf("R(");
				for (p = rstack+RSTACKSIZE-1; p >= rsp; p--)
					printf("%lx ", *p);
				printf(") ");
			#endif
			printf("[");
			fwrite(&w->name->data[0], 1, w->name->len & 0x7f, stdout);
			putchar(']');
			putchar('\n');
		#endif
		w->code(w);
	}
}

