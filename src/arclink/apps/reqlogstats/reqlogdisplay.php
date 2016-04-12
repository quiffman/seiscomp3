<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
 <?php
 require "../GEconfig.php";
 loadCMS();
?>
 <?php GEhead("EIDA Arclink Request Statistics", array("css=extra.css")); ?>

<body>
 <?php GEbar("GEOFON Earthquake Information Service"); ?>
 <?php GEmenu(); ?>
 <?php GEnavigation(); ?>
 <?php GEtop(); ?>

<?php

################ Common methods [liveseis_config.php]
function offset($date, $offset) {
	# $date - a 10-char yyyy-mm-dd string
	# $offset - in days
        $now = date("Ymd");

        ## There is no diff/add/sub on php 5.2
	$month = substr($date,5,2);
	$day = substr($date,8,2);
	$year = substr($date,0,4);
	$offset_sec = $offset*60*60*24;
        $n = date("Ymd", mktime(00,00,00,$month,$day,$year) + $offset_sec);

        if ((int)$n <= (int)$now)
                return date("Y-m-d", mktime(00,00,00,$month,$day,$year) + $offset_sec);

        return "";
}

class ReqLogSQ3 {
	private $db = "";
	private $filemtime = "";

	function __construct($file) {
		try {
			if (is_file($file)) {
				$this->db = new PDO('sqlite:'.$file); 
				$this->filemtime = filemtime($file);
			} else {
				echo('Database not found or is corrupted.');
				die();
			}
		} catch (Exception $e) {
			die($e->GetMessage());
		}
	}

       private function result2table($result, $key = "") {
	 //echo "Fetch[$key]:".$result->rowCount();
          $rows=array();
          if (empty($result)) return $rows;
          if (!empty($key))
            while ($r = $result->fetch()) {
             $k = $r[$key];
             unset($r[$key]);
             $rows[$k]=$r;
            }
          else
            while ($r = $result->fetch()) $rows[] = $r;
          return $rows;
        }

       function makequerytable($sql, $tname, $asTable = True) {
	 $q = str_replace('{table}', $tname, $sql);
	 $re = $this->db->query($q);
	 if (!$re) {
	   $error = $this->db->errorInfo();
	   echo "Command: " . $q . PHP_EOL;
	   echo "Error: " . $error[1] . ":" . $error[2] . PHP_EOL;
	 }
	 if (!$asTable) return $re;

	 //	 foreach ($re as $row) {
	 //  var_dump($row);
	 //}


	 return $this->result2table($re);
       }

        function makepreparetable($sql, $tname, $vars, $asTable = True) {
	  $q = str_replace('{table}', $tname, $sql);
	  $t = $this->db->prepare($q);
	  $re = $t->execute($vars);
	  if (!$re) {
	    $error = $this->db->errorInfo();
	    echo "Command: " . $q . PHP_EOL;
	    echo "Vars: " . print_r($vars) . PHP_EOL;
	    echo "Error: " . $error[1] . ":" . $error[2] . PHP_EOL;
	  }

	  //echo "Response:";
	  //var_dump($re);

	  if (!$asTable) return $re;

	  if (($re !== True) and ($re !== False)) {
	    return $re->fetchAll();
	  } else {
	    return $re;
	  }

	  return $this->result2table($re);
        }

	function timeStamp() {
		// Store mtime at time the db connection is opened.
		// Otherwise someone can update the file, but this
		// db connection sees an older version?
		return date("Y M d H:i:s", $this->filemtime);
	}
}

// -----------------------------------------------------------------------------

function h2($s) {
  return "<h2>$s</h2>";
}

function tag($tag, $s, $attrs = "") {
  $attributes = "";
  if (is_array($attrs)) {
      foreach ($attrs as $k => $v) {
	$attributes = $attributes . ' ' . $k .'="' . $v .'"';
      }
    }
  return "<$tag$attributes>$s</$tag>";
}

function tag_list($tag, $a) {
    $s = "";
    foreach ($a as $item) {
	$s = $s . tag($tag, $item);
    }
    unset($item);
    return $s;
}

function td($s) {
  return tag("td", $s);
}

function tr($s) {
  return tag("tr", $s);
}

function bytes_rounded($val) {
    // Return $val in units of kiB / MiB / GiB as appropriate,
    // as a *string*, with at most 3 decimal places.
    $byte_units = Array("kiB" => 1024, "MiB" => 1024*1024, "GiB" => 1024*1024*1024);
    foreach (Array("GiB", "MiB", "kiB") as $size) {
	if ($val > $byte_units[$size]) {
	    return rtrim(sprintf("%.3f", $val/$byte_units[$size]), "0") . " $size";
	}
    }
    return $val;
}

function sum_cols($data, $cols) {
    //  $data     - an Array of Arrays - one for header, one for data rows.
    //  $cols     - an Array of those column headers which are to
    //              be summed.

    $result = Array();
    foreach ($cols as $c => $v) {
	$result[$c] = 0;
    }

    //$headers = $data[0]; // First object
    $flipped_headers = array_flip($data[0]);
    $index = Array();
    $rounded = Array();
    foreach ($cols as $c => $v) {
	$index[$c] = $flipped_headers[$c];
	$rounded[$c] = False;
    }

    $byte_units = Array("kiB" => 1024, "MiB" => 1024*1024, "GiB" => 1024*1024*1024);

    foreach ($data[1] as $row) {
	foreach ($cols as $c => $v) {
	    if (array_key_exists($index[$c], $row)) {
		$val = $row[$index[$c]];
		if (substr($val, -2) == "iB") {
			$rounded[$c] = True;
			$unit = substr($val, -3);
			if (array_key_exists($unit, $byte_units)) {
				$val = floatval(substr($val, 0, -3))*$byte_units[$unit];
			} else {
				$val = 0;
			}
		}
		$result[$c] += $val;
	    }
	}
    }

    foreach ($cols as $c => $v) {
        if ($rounded[$c]) {
	    $result[$c] = bytes_rounded($result[$c]);
	}
    }
    return $result;
}

function render_table($format, $sources, $data, $options) {
    // Parameters:
    //  $format   - string, e.g. "html"
    //  $data     - an Array of Arrays - one for header, one for data rows.
    //  $options  - summable - an Array of column header names which
    //              are to be summed (across all rows i.e. vertically).

    if (! isset($data) ) return;

    if (count($data) > 0) {
	$headers = $data[0]; // First object
	echo '<table width="100%">';
	echo '  ' . tag('thead', tr(tag_list("th", $headers))) . PHP_EOL;
	echo PHP_EOL;
	echo '  <tbody>' . PHP_EOL;

	$rows = $data[1]; // Second object
	//for($i = 1, $size = count($data); $i < $size; ++$i) {
	//  $row = $data[$i];
	foreach ($rows as $row) {
	  $selected = Array();
	  foreach ($headers as $col) {
	    if (array_key_exists($col, $row)) {
	      if ($col == 'source') {
		$selected[$col] = $sources['labels'][$row[$col]];
	      } else {
		$selected[$col] = $row[$col];
	      }
	    } else {
	      $selected[$col] = "??";
	    }
	  };
	  echo '  ' . tr(tag_list("td", $selected)) . PHP_EOL;
	}
	if (array_key_exists("summable", $options)) {
	  $summable_cols = $options["summable"];
	  // var_dump($summable_cols);

	  $col_sums = Array();
	  $col_totals = sum_cols($data, $summable_cols);
	  foreach ($headers as $col) {
	    if (array_key_exists($col, $summable_cols)) {
	      $col_total = $col_totals[$col];
	      $col_sums[] = tag('span', $col_total, Array("class" => "col_total"));
	    } else {
		$col_sums[] = "&nbsp;";
	    };
	  }
	  $col_sums[0] = "Column sums";
	  echo '  ' . tr(tag_list("td", $col_sums)) . PHP_EOL;
	}
	echo '  </tbody>' . PHP_EOL;
	echo '</table>';
    } else {
	var_dump($data);
    };
 
}

// -----------------------------------------------------------------------------

function clean_date($text) {
  // ...must contain only numbers [0-9] and '-':
  // ...must be yyyy-mm-dd
  $text = strpbrk($text, '0123456789-');  // Ignore all before first digit.
  list($y, $m, $d) = sscanf($text, "%d-%d-%d"); // Ignores all after what matches.
  return sprintf("%04d-%02d-%02d", $y, $m, $d);
}

function mark_yesno($val) {
  if ($val != 0) {
    return tag('span', 'Y', Array('style' => 'color: green'));
  } else {
    return tag('span', 'N', Array('style' => 'color: red'));
  }
}

function render_availability_table($db, $start_str, $end_str) {
  $result = $db->makequerytable("SELECT id, host, port, dcid FROM `{table}`", "ArcStatsSource", True);
  $num_sources = count($result);
  $source_list = array();
  foreach ($result as $row) {
    $source_list[] = $row['id'];
  }
  //var_dump($source_list);


  // Workaround for prepare/execute query problem with wildcarded ?, ?
  // which gave "Column Index out of range".
  $q = "SELECT count(*) FROM {table} WHERE `start_day` = ':start' AND `source` = ':source'";
  //TODO $thing = $db->preparequery($q);

  // Headers for the table:
  $day_list = Array();
  //$day = DateTime::createFromFormat('Y-m-d', $start_str); // silly!
  $day = $start_str; 
  $blah = substr($day,0,7);
  $finish = offset($day, 30); // FIXME: Not smart enough for end of month/year

  $me="reqlogdisplay.php";
  for (; $day <= $finish; $day = offset($day, 1)) {
    $day_list[] = tag('a', substr($day,8,3), Array('href' => "$me?date=$day"));
  }

  echo h2("Days with reports in $blah");
  echo tag("p", "Reports included are from $num_sources sources between $start_str to $end_str:");

  echo "<table>";
  echo tag('thead', tr(td('EIDA NODE') . tag_list('th', $day_list)));

  //foreach ($source_list as $source) {
  foreach ($result as $row) {
    $source_id = $row['id'];
    //$source_id = $source['id'];
    $dcid = $row['dcid'];
    $host = $row['host'];
    $port = $row['port'];

    $day = $start_str;  // DateTime::createFromFormat('Y-m-d', $start_str);
    $presence_list = Array();
    for (; $day <= $finish; $day = offset($day, 1)) {
      $day_str = $day;
      //TODO $result = queryexecute($thing, Array($day_str, $source_id));
      //$result = $db->makepreparetable($q, 'ArcStatsSummary', Array($day_str, $source_id));
      $q_2 = str_replace(Array(':start', ':source'),
			 Array($day_str, $source_id),
			 $q);
      //print "QUERY: " . $q_2 . PHP_EOL;

      $result = $db->makequerytable($q_2, 'ArcStatsSummary');
      if (is_array($result)) {
	$ans = $result[0];
	$yesno = (intval($ans[0]) > 0);
      } else {
	$yesno = False;
      }
      $presence_list[] = mark_yesno($yesno);
    }
    $header = "$dcid"; // "$dcid ($host:$port)";
    echo tr(td($header) . tag_list('td', $presence_list)) . PHP_EOL;
  }

  echo "</table>";
}

// -----------------------------------------------------------------------------
date_default_timezone_set('UTC'); 

echo tag('h1', 'EIDA Arclink Request Statistics');

#echo "open connection to sqlite file: dba/__construct";
$db = new ReqLogSQ3('test4.db');

# May want to hide tables which reveal information about users:
$show_all_tables = isset($_GET['showall']);

$date_default = '2013-12-08'; 
$date_str = (isset($_GET['date']))?$_GET['date']:$date_default;
// "The GET variables are passed through urldecode()."
$date_str = clean_date($date_str);
//$date = DateTime::createFromFormat('Y-m-d', $date_str); // e.g. '2006-12-12'
// Check for failure with DateTime::getLastErrors(): TODO
$date = $date_str;
$start_day = $date;

$prev_str = offset($date, -1); 
$next_str = offset($date, 1);

$month_start = substr($date,0,7) . '-01';
$month_start_str = $month_start;
$month_end_str = substr($date,0,7) . '-31';
unset($month_start);


render_availability_table($db, $month_start_str, $month_end_str);

?>
<p>
  <a href="data/test.png"><img src="data/test.png" alt="Monthly chart total bytes" width="480" /></a>
  <a href="data/testh.png">Total_size by DCID</a>...
</p>

<?php

// echo tag("pick node or nodes to combine");

// The following must match the schema in reqlogstats.sql...
$table_list = Array( 'Summary', 'Request', 'Volume', 'Network', 'Messages' );
if ($show_all_tables) {
	$table_list[] = 'User';
	$table_list[] = 'UserIP';
	$table_list[] = 'ClientIP';
}

$table_heads = Array( 'Summary'  => Array('start_day', 'source', 'requests', 'requests_with_errors', 'error_count', 'users', 'stations', 'total_lines', 'total_size'),
		      'User'     => Array('start_day', 'source', 'userID', 'requests', 'lines', 'errors', 'size'),
		      'Request'  => Array('start_day', 'source', 'type', 'requests', 'lines', 'errors', 'size'),
		      'Volume'   => Array('start_day', 'source', 'type', 'requests', 'lines', 'errors', 'size'),
		      'Station'  => Array('start_day', 'source', 'streamID_networkCode', 'streamID_stationCode', 'stationID_locationCode', 'stationID_channelCode',  'requests', 'lines', 'errors', 'size', 'time'),
		      'Network'  => Array('start_day', 'source', 'networkCode', 'requests', 'lines', 'nodata', 'errors', 'size'),
		      'Messages' => Array('start_day', 'source', 'message', 'count'),
		      'UserIP'   => Array('start_day', 'source', 'userIP', 'requests', 'lines', 'errors', 'size'),
		      'ClientIP' => Array('start_day', 'source', 'clientIP', 'requests', 'lines', 'errors', 'size'),
		      );

// Node IDs come from:
//  SELECT * from ArcStatsSource where host= and port=
$selected_node_ids = Array( 1 );

echo tag('table', tr(tag_list('td', Array(tag('a', "&lt;&lt; Prev", Array('href' => 'reqlogdisplay.php?date=' . $prev_str)) . ' |',
					  "Summary statistics for ". $start_day,
					  '| ' . tag('a', "Next &gt;&gt;", Array('href' => 'reqlogdisplay.php?date=' . $next_str))
					  ))));
 

$marked_table_list=Array();
foreach ($table_list as $item) {
  $marked_table_list[] = tag("a", $item, Array('href' => "#Table$item"));
}
$table_nav = tag("ol", tag_list("li", $marked_table_list), Array('class' => 'nav'));
print tag("p", "Tables:") . $table_nav;


$q =  "SELECT id, host, port, dcid FROM `{table}` ORDER BY `id`";
$result = $db->makequerytable($q, 'ArcStatsSource');
$labels = Array();
foreach ($result as $row) {
  $labels[intval($row["id"])] = $row["dcid"];
}
$source_data = Array('labels' => $labels);

foreach ($table_list as $table) {
  echo h2("Table $table" . tag("a", "", Array('name' => "Table$table")));

    $options = Array();
    if (($table == "UserIP") || ($table == "ClientIP")) {
	$options["summable"] = array_flip(Array("requests", "lines", "errors"));
    } elseif ($table == "Summary") {
	$options["summable"] = array_flip(Array("requests", "requests_with_errors", "error_count", "total_lines", "total_size"));
    } elseif (($table == "User") || ($table == "Volume") || ($table == "Request")) {
        $options["summable"] = array_flip(Array("requests", "lines", "errors", "size"));
    } elseif ($table == "Network") {
        $options["summable"] = array_flip(Array("requests", "lines", "nodata", "errors", "size"));
    };

    foreach ($selected_node_ids as $node_id) {
      $order_col = $table_heads[$table][2];
      $q =  "SELECT * FROM `{table}` WHERE `start_day` = '?' ORDER BY `$order_col`";
      $tname = 'ArcStats' . $table;
      //$v = Array( $start_day );
      //$result = $db->makepreparetable($q, $tname, $v);

      $q_2 = str_replace('?', $start_day, $q);
      $result = $db->makequerytable($q_2, $tname);
      // echo "Got " . count($result) . " row(s)." ;
      $table_data = Array($table_heads[$table], $result);
      //var_dump($result);
      // TODO ... and combine this node's values
      echo PHP_EOL;
    };

    render_table('html', $source_data, $table_data, $options);
};

// One extra table:

$table = "Messages this month";
echo tag("h3", "Table $table" . tag("a", "", Array('name' => "TableMessages_this_month")));

foreach ($selected_node_ids as $node_id) {
  $order_col = "message" ; // $table_heads[$table][2];
  $date_constr = "WHERE start_day >= '$month_start_str' AND start_day <= '$month_end_str'";
  $q =  "SELECT * FROM `{table}` " . $date_constr . " ORDER BY `start_day`, `message`";
  $tname = 'ArcStatsMessages';

  $result = $db->makequerytable($q, $tname);
  // echo "Got " . count($result) . " row(s)." ;
  $table_data = Array($table_heads['Messages'], $result);
  //var_dump($result);
  echo PHP_EOL;
};

render_table('html', $source_data, $table_data, $options);

//var_dump($source_data);
//echo "close connection";

?>
 <?php GEbottom(); ?>
 <?php GEfooter(); ?>
</body>
</html>
