<?php

$file_in = $argv[1];
$file_out = $argv[2];
$file_flow = $argv[3];

if(empty($file_in) OR empty($file_out))
	exit("Usage : program <param1> <param2> <param3> \n");

$main_log_cnames = array('ExternalID', 'WMS_ID', 'senderAddress', 'Misbehavior', 'numPackets', 'NeighborSize', 'Entropy', 'SampledFlowSize', 'NumWSMs',
'AverageMacDelay', 'ReceivedBroadcasts', 'SentPackets', 'SlotsBackoff', 'SNIRLostPackets', 'TimesIntoBackoff', 'TooLittleTime', 
'totalBusyTime', 'TotalLostPackets', 'busyTime', 'totalTime');

$file_lines = file($file_in);
$i = 0;

foreach ($file_lines as $line) {

        $i++;

        if($i == 1)
	{
                $names = explode(",", $line);
		//print_r($names);
	}
        else if($i == 2)
	{
                $vals = explode(",", $line);
		//print_r($vals);
	}
}

$i = 0;

for($k = 0; $k < count($vals); $k++)
{
	$names[$k] = str_replace("/veins/examples/veins/results/nodebug-0.sca_nodebug-0-20190905-17:32:58-3240_RSUExampleScenario.node[", "Node[", $names[$k]);
	$names[$k] = str_replace("appl_", "", $names[$k]);

	$names[$k] = str_replace("nic.mac1609_4_", "", $names[$k]);
	$names[$k] = str_replace("nic.phy80211p_", "", $names[$k]);
	$names[$k] = str_replace("veinsmobility_", "", $names[$k]);
}

// pick and print column names

while($i < 29)
{
        $cname = $names[$i];
	$i++;

	// we want 9, 

	if($i < 9 OR $i == 10 OR $i == 11 OR $i == 13 OR $i == 14 OR $i == 23 OR $i == 24 OR $i == 25 OR $i == 26 OR $i == 27 OR $i == 28)
        	continue;

	list($nodename, $column) = explode(".", $cname);
	print $column.",";
}

$m = 0;
print "\n";
$post_stats = "";
$line_count = 0;

$handle = fopen($file_out, 'a');

$log_cnames = implode(",", $main_log_cnames);

fwrite($handle, $log_cnames."\n");

for($n = 0; $n < count($vals); $n++)
{
	$m++;

	if($m < 9 OR $m == 10 OR $m == 11 OR $m == 13 OR $m == 14 OR $m == 23 OR $m == 24 OR $m == 25 OR $m == 26 OR $m == 27 OR $m == 28)
		continue;

	print $vals[$n];

	// let's create a string to write to a final log file with flow_* data

	$post_stats = $post_stats.$vals[$n];

	if($m == 29)
        {
                $m = 0;

		// this means we finished reading another row and will begin to process the next vehicle
		$flow_file_name = $file_flow.$line_count;
		$flow_lines = file($flow_file_name); // this is just to read the flow data to combine with post_stats string

		foreach ($flow_lines as $fline)
		{
			$log_line = trim($fline).",".trim($post_stats)."\n";
			fwrite($handle, $log_line);
		}

		// let's increment the line count to proceed
		$line_count++;
		$post_stats = "";
                print " \n";
        }
	else
	{
		print ",";
		$post_stats = $post_stats.",";
	}
}

fclose($handle);

?>
