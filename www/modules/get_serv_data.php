
<?php
require_once("../classes/Socket.class.php");
require_once("../classes/Server.class.php");
try {
    $server = new Server(); 
    $clients = $server->get_client_list();
    $slotlist = $server->get_slot_list();
    $waitlist = $server->get_wait_list();
    $serverinfos = $server->get_infos();
    $logs = $server->get_log_list();
    $console = $server->get_console(10);
    
    $server_start = date("d/m/Y H:i:s", $serverinfos['time']);
    $serverps = exec("ps ax -o user,pid | grep ".$serverinfos['pid'], $output);
    
    $server_user = explode(" ", $serverps, 2);
    ?>
<div id="slots">
    <h1>Slots disponibles</h1>
    <?php
    foreach($slotlist as $id => $slot) {
        if(ISSET($slot["client"])) {

            $slot_start = date("H:i:s", $slot["start"]);
            $time_spent = time() - $slot["start"];
            if($time_spent < 60) {
                $time_spent_s = date("s\s\\e\c", $time_spent);
            }
            if($time_spent > 60 && $time_spent < 3600) {
                $time_spent_s = date("i\m\i\\ns\s\\e\c", $time_spent);
            }
            if($time_spent > 3600) {
                $time_spent_s = date("G\hi\m\i\\ns\s\\e\c", $time_spent);
            }
    ?>
    <div class="slot">
        <div class="slotclient">
            <p class="username"><?php echo $clients[$slot["client"]]["name"] . " (" . $clients[$slot["client"]]["ip"] . ")";?></p>
            <p class="streamtime">Streaming since <?php echo $slot_start . " (". $time_spent_s . ")"; ?></p>
            <p class="slotlinks">
                <a href="#">Listen</a> | 
                <a data-socket="<?php echo $clients[$slot["client"]]["sock"]; ?>" href="#" id="kick">Kick</a> | 
                <a href="#">Mute</a>

            </p>
        </div>
        <ul class="clientinfos">
            <li>PID: <?php echo $slot['pid']; ?></li>
            <li>Jitter: <?php echo $slot['jitt']; ?>ms</li>
            <li>Buffer: <?php echo $slot['buff']; ?>ms</li>
            <li>Bitrate (client): <?php echo $clients[$slot['client']]['rate']; ?>kbps</li>
        </ul>
        <p class="slotid"><?php echo "#".($id+1)." (". $slot["port"] .")"; ?></p>
    </div>

    <?php
        }
        else {
    ?>
    <div class="slot">
        <div class="slotclient">
            <p class="empty">- Slot vide -</p>
        </div>
        <ul class="clientinfos">
            <li>Jitter: <?php echo $slot['jitt']; ?>ms</li>
            <li>Buffer: <?php echo $slot['buff']; ?>ms</li>
        </ul>
        <p class="slotid"><?php echo "#".($id+1)." (". $slot["port"] .")"; ?></p>
    </div>
    <?php
        }
    }
    ?>
</div>
<div id="waitlist">
    <h1>Liste d'attente</h1>
    <?php
    foreach($waitlist as $idw => $waitingID) {
        if($waitingID != NULL) {
            $waiting = $clients[$waitingID];
            
            $time_spent = time() - $waiting['connex_time'];
            if($time_spent < 60) {
                $time_spent_s = date("s\s\\e\c", $time_spent);
            }
            if($time_spent > 60 && $time_spent < 3600) {
                $time_spent_s = date("i\m\i\\ns\s\\e\c", $time_spent);
            }
            if($time_spent > 3600) {
                $time_spent_s = date("G\hi\m\i\\ns\s\\e\c", $time_spent);
            }
            ?>
    <div class="waitline">
        <div class="waitline">
            <p class="name"><b><?php echo $waiting['name']; ?></b> (<?php echo $waiting['ip']; ?>)</p>
            <p class="waitlinks">
                <a data-socket="<?php echo $waiting['sock']; ?>" href="#" id="kick">Kick</a>
            </p>
            <p class="waittime">Attente depuis <?php echo $time_spent_s; ?></p>
        </div>
    </div>
            <?php           
        }
    }
    ?>
</div>
<div id="server">
    <h1>Serveur</h1>
    <div>
        <div class="serverarea">
            <h2>Actions</h2>
            <ul>
                <li><a href="#" id="shutdown_server">Arrêter le serveur</a></li>
                <li><a href="#" id="kick_allclients">Kicker tous les clients</a></li>
            </ul>
        </div>
        <div class="serverarea">
            <h2>Reseau</h2>
            <ul>
                <li><b>Connexions en cours:</b> <?php echo $serverinfos['clients']; ?></li>
                <li><b>IP du serveur:</b></li>
            </ul>
        </div>
        <div class="serverarea">
            <h2>Processus</h2>
            <ul>
                <li><b>PID:</b> <?php echo $serverinfos['pid']; ?></li>
                <li><b>Lancement:</b> <?php echo $server_start; ?></li>
                <li><b>Utilisateur:</b> <?php echo $server_user[0]; ?></li>
            </ul>
        </div>
        <div class="serverarea">
            <h2>Journaux</h2>
            <ul>
            <?php foreach($logs as $key => $value) { ?>
                <li><?php echo $value ?></li>
            <?php } ?>
            </ul>
        </div>
        <div class="serverarea" id="console">
            <h2>Console</h2>
            <pre>
<?php foreach($console as $key => $value) {
    echo $value;
} ?>
            </pre>
        </div>
    </div>
</div>
    <?php
}
catch (Exception $e) {
?>
<div id="server">
    <h1>Serveur</h1>
    <div>
        <div class="serverarea">
            <h2>Le serveur est arreté</h2>
            <ul>
                <li><a href="#" id="start_server">Démarrer le serveur</a></li>
            </ul>
        </div>
    </div>
    
</div>
<?php } ?>
