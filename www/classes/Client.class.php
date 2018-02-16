<?php

/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 * Description of Client
 *
 * @author VertiDesk
 */
class Client {
    //put your code here
    private $_id;
    private $_name;
    private $_ip;
    private $_socket;
    
    public function __construct(array $data) {
        $this->hydrate($data);
    }
    
    public function hydrate(array $data) {
        foreach($data as $key => $value) {
            $method = 'init'. $key;
            
            if(method_exists($this, $method)) {
                $this->$method($value);
            }
        }

    }
    
    /* ACCESSORS */
    
    public function get_id() {
        return $this->_id;
    }
    
    public function get_name() {
        return $this->_name;
    }
    
    public function get_ip() {
        return $this->_ip;
    }
    
    public function get_socket() {
        return $this->_socket;
    }
    
    /* INITIALIZATORS */
    
    private function init_id($id) {
        if(!is_int($id) || $id < 0) {
            trigger_error("id must be int > 0", E_USER_WARNING);
            return;
        }
        $this->_id = $id;
    }
    
    private function init_name($name) {
        $this->_name = $name;
    }
    
    private function init_ip($ip) {
       $this->ip = $ip;
    }
    
    private function init_socket($socket) {
        if(!is_int($socket) || $socket < 0) {
            trigger_error("socket must be int > 0", E_USER_WARNING);
            return;
        }
        $this->_socket = $socket;
    }
    
    /* MUTATORS */
    
    public function set_id($id) {
        if(!is_int($id) || $id < 0) {
            trigger_error("id must be int > 0", E_USER_WARNING);
            return;
        }
        $this->_id = $id;
    }
    
    public function set_name($name) {
        $this->_name = $name;
    }
    
    public function set_ip($ip) {
       $this->ip = $ip;
    }
    
    public function set_socket($socket) {
        if(!is_int($socket) || $socket < 0) {
            trigger_error("socket must be int > 0", E_USER_WARNING);
            return;
        }
        $this->_socket = $socket;
    }
}

class ClientManager {
    
}

?>