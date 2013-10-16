gmk_eval('TEST5 = $(subst ee,EE,feet on the street)');

gmk_expand('$(TEST1)') eq 'expansion' and
    gmk_expand('$(TEST2)') eq 'hi' and
    gmk_expand('$(TEST3)') eq 'hi there' and
    gmk_expand('$(TEST4)') eq 'hi there' and
    gmk_expand('$(TEST5)') eq 'fEEt on the strEEt'
	or die 'fail';

gmk_add_function('perlsub', undef, 1, 1);

sub perlsub
{
    'perlsub '.shift;
}

1;
