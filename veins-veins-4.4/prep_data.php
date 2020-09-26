<?php

$file_in = $argv[1];
$file_out = $argv[2];

if( empty($file_in) OR empty($file_out) )
	exit( "Usage: program <parameter1> <param2> \n" );

$file_lines = file($file_in);
$i = 0;

$handle = fopen($file_out, 'a');

foreach ($file_lines as $line) {

	$line = str_replace("NaN", "0", $line);
	$line = str_replace("nan", "0", $line);

	$row = explode(",", $line);

	if($row[0] == 'ExternalID')
		continue;

	if(($row[4] > 20 && $row[5] > 10 && $row[6] < 4 && $row[7] > 12) OR ($row[6] < 0.2 && $row[10] > 10) )
		$row[3] = 1;
	else
		$row[3] = 0;

	$clean_line = implode(",", $row);
	fwrite($handle, $clean_line);
}



fclose($handle);

?>
