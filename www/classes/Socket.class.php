<?php

class Socket {
    private $_socket;
    private $_address = "localhost";
    private $_port = 8377;
    private $_buffsize = 2048;
    
    
    public function __construct() {
        /* Démarrage du socket */
        $this->_socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP) or die("Creation du socket impossible");
        $result = socket_connect($this->_socket, $this->_address, $this->_port);
        
        socket_set_option($this->_socket,SOL_SOCKET, SO_RCVTIMEO, array("sec"=>3, "usec"=>0));
        if(!$result) {
            throw new Exception();
        }
        
    }
    
    public function disconnect() {
        /* Fermeture du socket */
    }
    
    public function read() {
        /* Mise en écoute du socket */
        $value = socket_read($this->_socket, $this->_buffsize, PHP_NORMAL_READ);
        return $value;
    
    }
    
    public function send($string) {
        /* envoi d'un message */
        socket_send($this->_socket, strval($string), $this->_buffsize, 0);
    }

    public function get_value($key) {
        /* Récupérer la valeur lue dans la fonction read en fonction de $key */
    }
    
    public function close() {
        socket_shutdown($this->_socket);
        socket_close($this->_socket);
        $this->_socket = NULL;
    }
}

?>