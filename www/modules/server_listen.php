<?php

require("../classes/Socket.class.php");
require("../classe/SocketListen.php");

$listen = new SocketListen();

if($listen->listen()) {
    echo "Nouvelles données";
}