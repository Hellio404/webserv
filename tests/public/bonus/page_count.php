<?php

session_start();

global $page_count;

// if there are no sessions, echo "You have not yet visited this page"
if (!isset($_SESSION['visited'])) {
    // set session variable
    $_SESSION['visited'] = 1;
    $page_count = 0;
} else {
    // increment the page count
    $page_count = $_SESSION['visited'];
    $_SESSION['visited'] = $page_count + 1;
}
?>

<html>
<head><title>
<?php if ($page_count > 0)
    echo("You have visited this page before!");
else
    echo("First time!");
?>
</title></head>
<body>
<center><h1>
<?php if ($page_count > 0)
    echo("You have visited this page before!");
else
    echo("You have never visited this page before!");
?>
</h1></center>
<?php if ($page_count > 0)
    echo("<hr><center><h2>Page count: " . $page_count . " times!</h1></center>");
?>
</body>
</html>


