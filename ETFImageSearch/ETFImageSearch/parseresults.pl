#!/usr/bin/perl

open(SCRIPT, "<", "htscript3.txt");
open(RESULTS, "<", "result3.txt");

while (<SCRIPT>) {
	@parts = split(/,/, $_);
	@parts2 = split(/,/, <RESULTS>);

	if ($parts[5] eq "combined") {
		$type = "Combined";
		$bins = $parts[2] * $parts[3] * $parts[4];
		$quant = "$parts[2]*$parts[3]*$parts[4]";
	} elsif ($parts[5] eq "split" && $parts[8] eq "0") {
		$type = "Split";
		$bins = $parts[2] + $parts[3] + $parts[4];
		$quant = "$parts[2]+$parts[3]+$parts[4]";
	} elsif ($parts[5] eq "split" && $parts[8] eq "1") {
		$type = "Split Cumul.";
		$bins = $parts[2] + $parts[3] + $parts[4];
		$quant = "$parts[2]+$parts[3]+$parts[4]";
	}

	$dist = ucfirst(lc($parts[9]));
	if ($dist eq "K_s") { $dist="Kolmogorov-S."; }
	if ($dist eq "Wed") { $dist="WED"; }
	if ($dist eq "Jsd") { $dist="JSD"; }
	if ($dist eq "Bray_curtis") { $dist="Bray-Curtis"; }
	if ($dist eq "Hist_int") { $dist="Hist. Int."; }
	if ($dist eq "Wave_hedges") { $dist="Wave-Hedges"; }

	print "$parts[1],$type,$bins,$quant,$dist," . ucfirst($parts[6]) . ",$parts[7],$parts2[1],$parts2[2],$parts2[3],$parts2[4]";
}

close(SCRIPT);
close(RESULTS);
