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
#! /usr/local/bin/perl5
#
$haveCachedNormal = 0;
$cachedNormal = 0.0;

sub RandomReal {
	local($r);
	$r = rand(2147483646) / 2147483647.0;
	($r == 0.0 ? $DBL_MIN : $r);
}

# double Erlang::operator()()
# {
#	  int k = int( (pMean * pMean ) / pVariance + 0.5 );
#	  k = (k > 0) ? k : 1;
#	  double a = k / pMean;
# 
#     double prod = 1.0;
#     for (int i = 0; i < k; i++) {
# 		prod *= pGenerator -> asDouble();
#     }
#     return(-log(prod)/a);
# }
$DBL_MIN = 2.2250738585072014e-308;
sub Erlang {
	local($mean, $variance) = @_;
	local($k) = (int (($mean, $mean) / $variance + 0.5));
	$k = 1 if ($k <= 0);
	local($a) = ($k / $mean);

	local($prod, $i) = (1.0, 0);
	for (; $i < $k; $i++) {
		$prod *= RandomReal;
	}
	-log($prod)/$a;
}

# 
# double Geometric::operator()()
# {
#     int samples;
#     for (samples = 1; pGenerator -> asDouble() < pMean; samples++);
#     return((double) samples);
# }
sub Geometric {
	local($mean) = @_;
	local($samples) = (1);
	while (RandomReal() < $mean) {
		$samples++;
	}
	$samples;
}

# 
# double HyperGeometric::operator()()
# {
#	  double z = pVariance / (pMean * pMean);
#	  double pP = 0.5 * (1.0 - sqrt((z - 1.0) / ( z + 1.0 )));
#     double d = (pGenerator -> asDouble() > pP) ? (1.0 - pP) :  (pP);
#     return(-pMean * log(pGenerator -> asDouble()) / (2.0 * d) );
# }
sub HyperGeometric {
	local($mean, $variance) = @_;
	local($z) = ($variance * 1.0 / ($mean * $mean));
	local($P) = (0.5 * (1.0 - sqrt(($z - 1.0) / ($z + 1.0))));
	local($d) = (RandomReal > $P ? (1.0 - $P) : $P);
	-$mean * log(RandomReal) / (2.0 * $d);
}

# 
# double NegativeExpntl::operator()()
# {
#     return(-pMean * log(pGenerator -> asDouble()));
# }
sub NegExp {
    local($mean) = @_;

    -$mean * log(RandomReal);
}

# 
# double Normal::operator()()
# {
#     
#     if (haveCachedNormal == 1) {
# 	haveCachedNormal = 0;
# 	return(cachedNormal * pStdDev + pMean );
#     } else {
# 	
# 	for(;;) {
# 	    double u1 = pGenerator -> asDouble();
# 	    double u2 = pGenerator -> asDouble();
# 	    double v1 = 2 * u1 - 1;
# 	    double v2 = 2 * u2 - 1;
# 	    double w = (v1 * v1) + (v2 * v2);
# 	    
# //
# //	We actually generate two IID normal distribution variables.
# //	We cache the one & return the other.
# // 
# 	    if (w <= 1) {
# 		double y = sqrt( (-2 * log(w)) / w);
# 		double x1 = v1 * y;
# 		double x2 = v2 * y;
# 		
# 		haveCachedNormal = 1;
# 		cachedNormal = x2;
# 		return(x1 * pStdDev + pMean);
# 	    }
# 	}
#     }
# }
# 

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


sub Poisson {
    local($mean) = @_;			# Only effective for values of $mean < ~700!

    local($Bound) = exp(-1.0 * $mean);
    local($Product) = RandomReal;
	local($Count) = 1;
    for (; $Product >= $Bound; $Product *= RandomReal) {
		$Count++;
	}
    $Count - 1;
}

# double LogNormal::operator()()
# {
#     return exp (this->Normal::operator()() );
# }
# 
# double Weibull::operator()()
# {
#	  pInvAlpha = 1.0 / pAlpha;
#     return( pow(pBeta * ( - log(1 - pGenerator -> asDouble()) ), pInvAlpha) );
# }
# 
sub Weibull {
	local($alpha, $beta) = @_;
	pow($beta * -log(1.0 - RandomReal), 1.0/$alpha);
}

sub printBuckets {
	local($maxBucket, $i) = @_;
	for ($i = 0; $i < $#p; $i++) {
		if ($p[$i]) {
			printf "%-3d: %s\n", $i, '*' x ($p[$i] * 75.0 / $maxBucket + 0.5);
		}
		else {
			printf "%-3d\n", $i;
		}
	}
}


$scriptName = $0;
sub Usage {
	printf "%s <mean>\n", $scriptName;
	die;
}

if ($#ARGV < 0 || $#ARGV > 1 || $ARGV[0] =~ /^-(.)(.*)/) {
	printf "%s: Invalid argument(s): %s\n", $scriptName, $2;
	Usage;
}

($mean, $variance) = @ARGV;

srand();
$| = 1;

print "Normal\n";
undef @p;
$maxBucket = 0;
for ($i = 0; $i < 5000; $i++) {
	if (++$p[Normal($mean, $variance)] > $maxBucket) {
		$maxBucket++;
	}
}
printBuckets($maxBucket);

print "Erlang\n";
undef @p;
$maxBucket = 0;
for ($i = 0; $i < 5000; $i++) {
	if (++$p[Erlang($mean, $variance)] > $maxBucket) {
		$maxBucket++;
	}
}
printBuckets($maxBucket);

print "Geometric\n";
undef @p;
$maxBucket = 0;
for ($i = 0; $i < 5000; $i++) {
	if (++$p[Geometric(($mean > 1 ? 1/$mean : $mean))] > $maxBucket) {
		$maxBucket++;
	}
}
printBuckets($maxBucket);

print "Negative Exponential\n";
undef @p;
$maxBucket = 0;
for ($i = 0; $i < 5000; $i++) {
	if (++$p[NegExp($mean)] > $maxBucket) {
		$maxBucket++;
	}
}
printBuckets($maxBucket);

print "Poisson\n";
undef @p;
$maxBucket = 0;
for ($i = 0; $i < 5000; $i++) {
	if (++$p[Poisson($mean)] > $maxBucket) {
		$maxBucket++;
	}
}
printBuckets($maxBucket);
