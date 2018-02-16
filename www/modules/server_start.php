<h1>Démarrer le serveur</h1>
    <?php
$path = realpath("../..");
$jitter = 16;
$port = 1350;
$device = "default";
$nb_slots = 1;

if(ISSET($_POST['slots']) or ISSET($_POST['port']) or ISSET($_POST['jitter'])) {
    
    if(ISSET($_POST['slots']) && is_numeric($_POST['slots']) && $_POST['slots'] > 0) {
        $nb_slots = escapeshellcmd($_POST['slots']);
    }
    if(ISSET($_POST['port']) && is_numeric($_POST['port']) && $_POST['port'] > 0) {
        $port = escapeshellcmd($_POST['port']);
    }
    if(ISSET($_POST['jitter']) && is_numeric($_POST['jitter']) && $_POST['jitter'] > 0) {
        $jitter = escapeshellcmd($_POST['jitter']);
    }

   
    exec($path."/./rx -i ".$nb_slots." -p ".$port." -j ".$jitter . " > /dev/null &");
    ?>

    <img src="public/img/loading.gif" alt="waiting"/>
<?php
}
else {
    
?>
<div class="slot">
    <form id="serv_start">
        <input type="hidden" id="serv_path" name="serv_path" value="<?php echo $path; ?>" />
        <p><label for="form_nb_slots">Nombre de slots: </label><input type="number" name="form_nb_slot" id="form_nb_slot" min="1" value=1 /></p>
        <p><label for="form_port">Port de début:</label><input type="text" name="form_port" id="form_port" value="<?php echo $port; ?>" maxlength="5"/></p>
        <p><label for="form_jitter">Jitter (ms):</label><input type="text" name="form_jitter" id="form_jitter" value="<?php echo $jitter; ?>" maxlength="5"/></p>
        <p>Aperçu de la commande:</p>
        <p class="streamtime" id="command">
            <?php echo $path . "/./rx -i " . $nb_slots . " -j " . $jitter . " -p ".$port.""; ?>
        </p>
        <p><label></label><input type="submit" id="valider_serv" /><p>
    </form>
    
</div>
<?php
}
?>