<?

$filez = array_fill(1, 1400, "");

$file = file("annotation.txt");
$out = fopen("images/categories.txt", "w");
foreach ($file as $i) {
	$parts = preg_split("/\s/", $i);
	$oldfn = $parts[0].".jpg";
	$newfn = str_replace("/", ".", $oldfn);
	if (!file_exists($oldfn)) {
		$oldfn = $parts[0].".JPG";
	}
	if (!file_exists($oldfn)) {
		// Convert from GIF
		$oldfn = $parts[0].".gif";
		if (file_exists($oldfn)) {
			system("convert $oldfn images/$newfn");
		}
	}
	else
		system("cp $oldfn images/$newfn");
	unset($parts[0]);
	fwrite($out, "$newfn ".join(" ", $parts) . "\n");
}
fclose($out);

?>
