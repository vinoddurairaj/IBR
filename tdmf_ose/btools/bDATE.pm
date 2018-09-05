##############################################
##############################################
# 
#   bDATE.pm
#
#	package module to create YYYYMMDD format
#	hopefully.....
#
#	added get_time sub routine 2003/01/09
#
##############################################
##############################################

############################################
############################################
# get_date
############################################
############################################

sub get_date {

#print "\n\t from sub get_date\n";

($DAY,$MONTH,$YEAR) = (localtime)[3,4,5];

$YEAR=$YEAR+1900;
$MONTH=$MONTH+1;

if ( "$MONTH" !~ m/(\d)(\d)/ ) {
   #print "\nIF :  MONTH format ok\n";
   $MONTH = "0$MONTH";
}

if ( "$DAY" !~ m/(\d)(\d)/ ) {
   #print "\nIF :  DAY format ok\n";
   $DAY = "0$DAY";
}

push (@date_out, "$MONTH");
push (@date_out, "$YEAR");
push (@date_out, "$DAY");

#print "\nYEARMONTHDAY = \t $YEAR$MONTH$DAY\n\n";
#print "\ndate_out =\n@date_out\n\n";

return(@date_out);

}  # close bracket get_date

############################################
############################################
# get_time
############################################
############################################

sub get_time {

#print "\n\t from sub get_time\n";

($MINUTE,$HOUR) = (localtime)[1,2];

push (@time_out, "$MINUTE");
push (@time_out, "$HOUR");

#print "\nYEARMONTHDAY = \t $YEAR$MONTH$DAY\n\n";
#print "\ndate_out =\n@date_out\n\n";

return(@time_out);

}  # close bracket get_time

##############################################
##############################################
