<?php
    require("../classes/Socket.class.php");
    require("../classes/Server.class.php");
    
    if(isset($_GET['socket'])) {
        if($_GET['socket'] == 'all') {
            $server = new Server();
            $result = $server->kick_all_clients();

            $server->close();
        }
        else {
            $socket = intval($_GET['socket']);

            $server = new Server();
            $result = $server->kick_client($socket);

            $server->close();
        }
    }
    if($result) {
        echo "Success";
    }
    else {
        echo "Fail";
    }
?>