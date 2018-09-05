#
# Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
#
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
# Government is subject to restrictions as set forth in
# subparagraph (c)(1)(ii) of the Rights in Technical Data and
# Computer Software clause at DFARS 52.227-7013 and in similar
# clauses in the FAR and NASA FAR Supplement.
#
#

sub RandomReal {
	local($r);
	$r = rand(2147483646) / 2147483647.0;
	($r == 0.0 ? $DBL_MIN : $r);
}

sub Erlang {
	local($mean, $variance) = @_;
	local($k) = (int (($mean * $mean) / $variance + 0.5));
	$k = 1 if ($k <= 0);
	local($a) = ($k / $mean);

	local($prod, $i) = (1.0, 0);
	for (; $i < $k; $i++) {
		$prod *= RandomReal;
	}
	-log($prod)/$a;
}

sub Geometric {
	local($mean) = @_;
	local($samples) = (1);
	while (RandomReal() < $mean) {
		$samples++;
	}
	$samples;
}

sub HyperGeometric {
	local($mean, $variance) = @_;
	local($z) = ($variance * 1.0 / ($mean * $mean));
	local($P) = (0.5 * (1.0 - sqrt(($z - 1.0) / ($z + 1.0))));
	local($d) = (RandomReal > $P ? (1.0 - $P) : $P);
	-$mean * log(RandomReal) / (2.0 * $d);
}

sub NegExp {
    my($mean) = @_;
	my($r);
	do {
		$r = RandomReal;
	} while ($r == 0.0);

    -$mean * log($r);
}


sub Normal {
	local($mean, $variance) = @_;
	local($stdDev) = (sqrt($variance));
	local($result);

    if ($haveCachedNormal) {
		$haveCachedNormal = 0;
		$result = $cachedNormal * $stdDev + $mean;
    }
	else {
		for(;;) {
			$u1 = RandomReal();
			$u2 = RandomReal();
			$v1 = 2.0 * $u1 - 1.0;
			$v2 = 2.0 * $u2 - 1.0;
			$w = ($v1 * $v1) + ($v2 * $v2);

			# We actually generate two IID normal distribution variables.
			# We cache the one & return the other.
			#
			if ($w <= 1) {
				$y = sqrt( (-2.0 * log($w)) / $w);
				$x1 = $v1 * $y;
				$x2 = $v2 * $y;

				$haveCachedNormal = 1;
				$cachedNormal = $x2;
				$result = $x1 * $stdDev + $mean;
				last;
			}
		}
	}
	$result;
}

sub Weibull {
	local($alpha, $beta) = @_;
	pow($beta * -log(1.0 - RandomReal), 1.0/$alpha);
}

1;
