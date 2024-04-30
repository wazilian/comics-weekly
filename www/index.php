
<html>
  <head>
    <link rel="stylesheet" href="style.css">
  </head>
  <body>
    <div id="title">Comics Weekly</div>
    <div id="tables">

<?php

$link_prepend = "https://metron.cloud/issue/";
$output_dir = "/var/www/html/txt";
$cmp_date = "Y-m-d";
$display_date = "M j, Y";

/* grab the dates for the Wednesday of the previous, current, and next weeks */
$store_dates = Array(
  date($cmp_date, strtotime("Wednesday last week")) => "Last Week",
  date($cmp_date, strtotime("Wednesday this week")) => "This Week",
  date($cmp_date, strtotime("Wednesday next week")) => "Next Week"
);

/* loop through each text file (3) and tabularly print the issue list */
foreach ($store_dates as $date => $str) {
  $lines = file($output_dir . "/" . $date . ".txt", FILE_IGNORE_NEW_LINES);

  echo "<table>";
  echo "<tr>";
  echo "<td class='table_header'>" . $str . " " . date($display_date, strtotime($date)) . "</td>";
  echo "</tr>";

  foreach ($lines as $issue) {
    /* create this issue's link */
    $issue_link = preg_replace("/[()#!':,.\/]/", "", $issue);
    $issue_link = preg_replace("/\s+/", "-", $issue_link);
    $issue_link = $link_prepend . strtolower($issue_link) . "/";

    echo "<tr><td><a target='_blank' href='" . $issue_link . "'>" . $issue . "</a></td></tr>";
  }

  echo "</table>";
}

?>

    </div>
  </body>
</html>
