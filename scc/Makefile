archs	+= arm
archs	+= arm64

V	:= @
Q	?= $(V:1=)
CC	:= gcc
CFLAGS	?= -Wall -Wwrite-strings -g
HOSTCC	:= $(CC)

CPPFLAGS+= -I..

#####
cg-objs		:= codegen.o insncode.o parse-smd.o

host-objs	:= $(cg-objs)
gen-srcs	:= $(archs:%=gen-%.c)
gen-objs	:= $(archs:%=gen-%.o)
pgms		:= $(archs:%=test-%)
lib-objs	:= translate.o compile.o

objs-all	:= $(host-objs) $(lib-objs) $(gen-objs)
#####

all: libscc.a $(gen-srcs) $(gen-objs) codegen $(pgms)

libscc.a: $(lib-objs)
	@echo "  AR      $@"
	$(Q)$(AR) rcs $@ $^

test-%: gen-%.o $(lib-objs) ../libsparse.a
	@echo "  LD      $@"
	$(Q)$(CC) $(LDFLAGS) -o $@ $^

gen-%.o: gen-%.c gen-common.c	# is included into the generated file

gen-%.c: %.smd codegen
	@echo "  GEN     $@"
	$(Q)./codegen $* $< > $@

codegen: $(cg-objs)
	@echo "  HLD     $@"
	$(Q)$(HOSTCC) $(LDFLAGS) -o $@ $^

$(host-objs): %.o: %.c
	@echo "  HCC     $@"
	$(Q)$(HOSTCC) $(CPPFLAGS) $(CFLAGS) -Wp,-MD,.$@.d -c -o $@ $<

%.o: %.c
	@echo "  CC      $@"
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) -Wp,-MD,.$@.d -c -o $@ $<

%.sc: %.c
	@echo "  CHECK   $@"
	$(Q)../cgcc $(CPPFLAGS) $(CFLAGS) $(CHECKER_FLAGS) -c $<

%.c: %.y
	@echo "  GEN     $@"
	$(Q)bison -o $@ $^

selfcheck: $(objs-all:%.o=%.sc)

-include $(objs-all:%.o=.%.o.d)
