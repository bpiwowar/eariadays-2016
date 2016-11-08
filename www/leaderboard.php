<h1>Leaderboard</h1>
<table>
<thead>
<tr><th>Name</th><th>AP</th><th>PC10</th></tr>
</thead>
<tbody>
<?php

$sql = new SQLite3("/Users/bpiwowar/temporary/ClueWeb12B/leaderboard/db.sqlite", SQLITE3_OPEN_READONLY);
$result = $sql->query("SELECT name, pc10, ap FROM SCORES ORDER BY ap DESC");
$result->reset();
while ($r = $result->fetchArray()) {
	 print "<tr><td>{$r["NAME"]}</td><td>{$r["AP"]}</td><td>{$r["PC10"]}</td></tr>";
}

?>
</tbody>
</table>