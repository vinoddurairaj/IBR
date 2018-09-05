#!/usr/bin/perl -I..
##############################################
##############################################
# 
#   bDATE.pm
#
#    subs to generate time / date / GMT 
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

if ( "$MINUTE" !~ m/(\d)(\d)/ ) {
   #print "\nIF :  MINUTE format ok\n";
   $MINUTE = "0$MINUTE";
}

if ( "$HOUR" !~ m/(\d)(\d)/ ) {
   #print "\nIF :  HOUR format ok\n";
   $HOUR  = "0$HOUR";
}

push (@time_out, "$MINUTE");
push (@time_out, "$HOUR");

#print "\nYEARMONTHDAY = \t $YEAR$MONTH$DAY\n\n";
#print "\ndate_out =\n@date_out\n\n";

return(@time_out);

}  # close bracket get_time

############################################
############################################
# get_GMT_date
############################################
############################################

sub get_GMT_date {

#print "\n\t from sub get_GMT_date\n";

($GMTSECOND,$GMTMINUTE,$GMTHOUR,$GMTDAY,$GMTMONTH,$GMTYEAR) = (gmtime)[0,1,2,3,4,5];

$GMTYEAR=$GMTYEAR+1900;
$GMTMONTH=$GMTMONTH+1;

if ( "$GMTMONTH" !~ m/(\d)(\d)/ ) {
   #print "\nIF :  GMTMONTH format ok\n";
   $GMTMONTH = "0$GMTMONTH";
}

if ( "$GMTDAY" !~ m/(\d)(\d)/ ) {
   #print "\nIF :  GMTDAY format ok\n";
   $GMTDAY = "0$GMTDAY";
}

push (@gmt_out, "$GMTMONTH");
push (@gmt_out, "$GMTYEAR");
push (@gmt_out, "$GMTDAY");

#print "\nGMTYEARMONTHDAY = \t $GMTYEAR$GMTMONTH$GMTDAY\n\n";
#print "\ngmt_out =\n@gmt_out\n\n";

return(@gmt_out);

}  # close bracket get_GMT_date

##############################################
##############################################
