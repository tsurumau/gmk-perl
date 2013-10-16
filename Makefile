ifdef osx
    LDFLAGS = -bundle -Wl,-undefined -Wl,dynamic_lookup
else
    LDFLAGS = -shared -fPIC
endif

all: perl.so

perl.so: perl.c
	$(CC) $(LDFLAGS) -o $@ $< $(shell $(PERL) -MExtUtils::Embed -e ccopts -e ldopts)

test-main: perl.so
	$(call assert-equal,3,$(perl 1+2))
	$(call assert-equal,1234,$(perl 1234))
	$(call assert-equal,foo,$(perl "foo"))
	$(call assert-equal,"1234 'foo","$(perl "1234 'foo")")
	$(call assert-equal,Just Another Perl Hacker,$(perl reverse 'rekcaH lreP rehtonA tsuJ'))
	$(call assert-equal,expansion,$(perl gmk_expand '$$(TEST1)'))
	$(call assert-equal,hi,$(TEST2))
	$(call assert-equal,hi there,$(TEST3))
	$(call assert-equal,hi there,$(TEST4))
	$(call assert-equal,55,$(perl fib 10))
	$(call assert-equal,55,$(fib 10))
	$(call assert-equal,perl 1 2 3,$(perl-func 1 2 3))
	$(call assert-equal,fEEt on the strEEt,$(TEST5))

.PHONY : test
test: test-main
	@echo $(call perl-free)

ifeq ($(MAKECMDGOALS),test)
load perl.so

TEST1 = expansion

TEST2 = $(perl gmk_expand "$(shell echo hi)")

TEST3_ = bye
EVAL = TEST3 = $(TEST3_) $(shell echo there)
$(perl gmk_eval '$(value EVAL)')
TEST3_ = hi

TEST4_ = hi $(shell echo there)
TEST4 = $(perl gmk_expand '$(TEST4_)')

define fib
sub fib {
  my $$n = shift;
  $$n == 0 ? 0 : $$n == 1 ? 1 : fib($$n-1) + fib($$n-2)
}
endef
$(perl $(fib))

$(perl gmk_add_function 'fib')

$(perl gmk_add_function 'perl-func', sub {"perl @_"})

$(perl-load ./testmk.pl)
endif

define assert-equal =
$(warning $(1))
$(warning $(2))
$(if $(shell test $(call escape,$(1)) = $(call escape,$(2)) && echo 1),$(warning ok),$(warning ng))
endef

escape = '$(escape-quote)'
escape-quote = $(subst ','\'',$(1))
