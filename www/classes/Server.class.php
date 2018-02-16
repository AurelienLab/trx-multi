<?php

class Server extends Socket {
    //put your code here
    
    public function get_client_list() {
        /* Retourne un array de clients */
        $this->send("get_clients");
        $clients = unserialize($this->read());
        
        return $clients;
    }
    
    public function get_slot_list() {
        /* Retourne un array de slots */
        $this->send("get_slot_list");
        $slotlist = unserialize($this->read());
        
        return $slotlist;
    }
    
    public function get_wait_list() {
        $this->send("get_wait_list");
        $waitlist = unserialize($this->read());
        
        return $waitlist;
    }
    
    public function get_infos() {
        $this->send("get_server_infos");
        $infos = unserialize($this->read());
        
        return $infos;
    }
    
    public function get_log_list() {
        
        if($folder = opendir('../../logs')) {
            while(false !==($file = readdir($folder))) {
                
                if(stripos($file, "rx") !== FALSE) {
                    $result[] = $file;
                }
            }
        }
        rsort($result);
        return $result;
    }
    
    public function get_console($lines) {
        //$file = fopen('../../logs/'.$this->get_log_list()[0], 'r');
        $file = file('../../logs/'.$this->get_log_list()[0]);
        $nb_line = count($file);
        
        for($i=($nb_line-$lines);$i<$nb_line;$i++) {
            $result[] = $file[$i];
        }
        fclose($file);
        return $result;
    }
    
    public function kick_client($socket) {
        /* Envoie la commande de kick au serveur */
        if(is_int($socket)) {
            $this->send("kick_client ".$socket);
        }
        if($this->read() == "success") {
            return 1;
        }
        else { return 0; }
    }
    
    public function kick_all_clients() {
        $this->send("kick_all_clients");
        
        return 1;
    }
    
    public function shutdown() {
        $this->send("shutdown");
    }
}

?>