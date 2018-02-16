<?php
require("../classes/Socket.class.php");
require("../classes/Server.class.php");

$server = new Server();

$server->shutdown();

$server->close();

?>