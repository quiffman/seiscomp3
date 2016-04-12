<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
 <?php
 // require "../GEconfig.php";
 require "../../../geofon/GEconfig.php";
 loadCMS();
?>
 <?php GEhead("Arclink Request Statistics",
	array("css=extra.css", "css=/eida/local_basic.css")); ?>

<body>
 <?php GEbar("Arclink Request Statistics"); ?>
 <?php GEmenu(); ?>
 <?php GEnavigation(); ?>
 <?php GEtop(); ?>

<?php

require 'reqlog.php';

// -----------------------------------------------------------------------------

function clean_code($text) {
  // ...must be alphanumeric and uppercase: only [A-Z] and numbers [0-9] and '/' or ':':
  return strtoupper(substr($text, 0, 8));
}

function clean_date($text) {
  // ...must contain only numbers [0-9] and '-':
  // ...must be yyyy-mm-dd
  $text = strpbrk($text, '0123456789-');  // Ignore all before first digit.
  list($y, $m, $d) = sscanf($text, "%d-%d-%d"); // Ignores all after what matches.
  return sprintf("%04d-%02d-%02d", $y, $m, $d);
}


// -----------------------------------------------------------------------------
date_default_timezone_set('UTC'); 


# May want to hide tables which reveal information about users:
$show_all_tables = isset($_GET['showall']);

$date_default = offset(date('Y-m-d'), -2);  // Two days ago

$date_str = (isset($_GET['date']))?$_GET['date']:$date_default;

$code_default = 'GE';
$code_str = (isset($_GET['code']))?$_GET['code']:$code_default;

// "The GET variables are passed through urldecode()."
$date_str = clean_date($date_str);
//$date = DateTime::createFromFormat('Y-m-d', $date_str); // e.g. '2006-12-12'
// Check for failure with DateTime::getLastErrors(): TODO

$code_str = clean_code($code_str);

// Should be valid from here.

$date = $date_str;
$start_day = $date;

$prev_str = offset($date, -1); 
$next_str = offset($date, 1);

$month_start = substr($date,0,7) . '-01';
$month_start_str = $month_start;
$month_end_str = substr($date,0,7) . '-31';
unset($month_start);

#echo "open connection to sqlite file: dba/__construct";
$year = substr($date,0,4);
$month = substr($date,5,2);

$db = new ReqLogSQ3($reqlogstats_db_dir . "/reqlogstats-$year.db");
echo tag('h1', 'EIDA Arclink Request Statistics for network ' . $code_str);

// echo tag("pick node or nodes to combine");

// The following must match the schema in reqlogstats.sql...
$table_list = Array( 'Networks' );

$table_heads = Array(
   'Network'  => Array('start_day', 'source', 'networkCode', 'requests', 'lines', 'nodata', 'errors', 'size'),
   'Networks' => Array('networkCode', 'days', 'requests', 'lines', 'nodata', 'errors', 'size'),
);

// Node IDs come from:
//  SELECT * from ArcStatsSource where host= and port=
$selected_node_ids = Array( 1 );

// FIXME
$prev_month_str = "2013-11-31";
$next_month_str = "2014-01-01";

$items = Array( tag('a', "&lt;&lt; Prev month", Array('href' => 'reqlogdisplay.php?date=' . $prev_month_str)) . ' |',
                "Monthly summary statistics for ". $start_day,
                '| ' . tag('a', "Next month &gt;&gt;", Array('href' => 'reqlogdisplay.php?date=' . $next_month_str))
);
echo tag('table', tr(tag_list('td', $items)));
 

$marked_table_list=Array();
foreach ($table_list as $item) {
  $marked_table_list[] = tag("a", $item, Array('href' => "#Table$item"));
}
$table_nav = tag("ol", tag_list("li", $marked_table_list), Array('class' => 'nav'));
// print tag("p", "Tables:") . $table_nav;

# ---------------------

$img_base_url = "../data";
$img1 = "$img_base_url/graph-$code_str-sources.svg";
$img2 = "$img_base_url/graph-$code_str-total.svg";

echo '<div style="padding:2ex 0px;">';
echo '<img alt="Bytes for this network, by source" src="' . $img1 . '" width="480" />';
echo '<img alt="Total bytes for this network" src="' . $img2 . '" width="480" />';
echo '</div>';

// Source data is needed for all tables:

$q = "SELECT id, host, port, dcid FROM `{table}` ORDER BY `id`";
$result = $db->makequerytable($q, 'ArcStatsSource');
$labels = Array();
foreach ($result as $row) {
  $labels[intval($row["id"])] = $row["dcid"];
}
$source_data = Array('labels' => $labels);

// One extra table:

$table = "Requests for $code_str this month";
echo tag("h3", "Table $table" . tag("a", "", Array('name' => "TableReq_this_month" . $code_str)));

foreach ($selected_node_ids as $node_id) {
  $date_constr = "WHERE start_day >= '$month_start_str' AND start_day <= '$month_end_str'";
  $net_constr = "networkCode='$code_str'";
  $q =  "SELECT * FROM `{table}` " . $date_constr . " AND " . $net_constr . " ORDER BY `start_day`, `src`";
  $tname = 'ArcStatsNetwork';

  $result = $db->makequerytable($q, $tname);
  // echo "Got " . count($result) . " row(s)." ;
  $table_data = Array($table_heads['Network'], $result);
  //var_dump($result);
  echo PHP_EOL;
};

$options["summable"] = array_flip(Array("requests", "lines", "nodata", "errors", "size"));
$options["linkcodes"] = False;
render_table('html', $source_data, $table_data, $options);

# ---------------------


foreach ($table_list as $table) {
  echo h2("Table $table over the month" . tag("a", "", Array('name' => "Table$table")));

    $options = Array();

    foreach ($selected_node_ids as $node_id) {
      $date_constr = "WHERE start_day >= '$month_start_str' AND start_day <= '$month_end_str'";
      $q = "SELECT `networkCode`, COUNT(*) as `days`, SUM(requests) as `requests`, SUM(lines) as `lines`, SUM(nodata) as `nodata`, SUM(errors) as `errors`, SUM(size) as `size` FROM {table} " . $date_constr ." GROUP BY `networkCode` ORDER BY `networkCode`";
      $tname = 'ArcStatsNetwork';
      $result = $db->makequerytable($q, $tname);
      // echo "Got " . count($result) . " row(s)." ;
      $table_data = Array($table_heads[$table], $result);
      //var_dump($result);
      // TODO ... and combine this node's values
      echo PHP_EOL;
    };

    $options["summable"] = array_flip(Array("requests", "lines", "nodata", "errors", "size"));
    $options["linkcodes"] = True;
    render_table('html', $source_data, $table_data, $options);
};


?>
 <?php GEbottom(); ?>
 <?php GEfooter(); ?>
</body>
</html>
