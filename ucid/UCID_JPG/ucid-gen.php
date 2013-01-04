<?

$data = file("UcidQrels.txt");
$result = array();
foreach($data as $line) {
	$bits = split(" ", $line);
	if (isset($result[$bits[2]]))
		$result[$bits[2]] .= " " . $bits[0];	
	else
		$result[$bits[2]] = $bits[0];	
}

ksort($result);

foreach ($result as $key => $value) {
	print "$key".".jpg $value\n";
}

?>