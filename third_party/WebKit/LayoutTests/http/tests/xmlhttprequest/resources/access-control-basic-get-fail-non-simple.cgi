#!/usr/bin/perl -wT
use strict;

my $request;

if ($ENV{'REQUEST_METHOD'} eq "GET") {
    print "Content-Type: text/plain\n";
    print "Access-Control-Credentials: true\n";
    print "Access-Control-Origin: http://127.0.0.1:8000\n\n";
    print "FAIL: Cross-domain access allowed.\n";
}
