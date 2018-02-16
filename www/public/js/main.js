$(document).ready(function(){ 
    /* Rehcargement de la partie principale */
    var reload = 1;
    function load_data() {
            setTimeout( function() {
                if(reload) {
                    $.ajax({
                        url : '/modules/get_serv_data.php',
                        type : 'GET',
                        dataType : 'html',
                        success : function(code_html, statut){
                            $("#content").html($(code_html));
                        }
                    });

                    load_data();
                }
            }, 3000);
    }

    load_data();


    /* Demande de kick client */
    $("div#content").on('click', '#kick', function(){
        event.preventDefault();
        if(confirm("Voulez-vous vraiment kicker un utilisateur ?")) {

            var socket = $(this).attr('data-socket');

            $.ajax({
                url : '/modules/kick_client.php',
                type : 'GET',
                data : 'socket=' + socket,
                dataType : 'text'
            });
        }
    });
    
    $("div#content").on('click', '#kick_all', function(){
        event.preventDefault();
        if(confirm("Voulez-vous vraiment kicker tous les utilisateurs ?")) {

            $.ajax({
                url : '/modules/kick_client.php',
                type : 'GET',
                data : 'socket=all',
                dataType : 'text'
            });
        }
    });
    
    /* Demande d'arrêt serveur */
    $("div#content").on('click', '#shutdown_server', function(){
        event.preventDefault();
        if(confirm("Voulez-vous réellement arrêter le serveur ?")) {
            $.ajax({
                url: '/modules/shutdown_server.php',
                type : 'GET',
                dataType : 'html'
            });
        }
    });

    /* Demande de démarrage de serveur */
    $("div#content").on('click', '#start_server', function(){
        event.preventDefault();


        $.ajax({
            url : '/modules/server_start.php',
            type : 'GET',
            dataType : 'html',
            success : function(code_html, statut){
                reload = 0;
                $("#server").html($(code_html));
            }
        });
    });

    var serv_path;
    var nb_slots;
    var port;
    var jitter;

    $("div#content").on('hover', '#serv_start',function(){
        serv_path = $("#serv_path");
        nb_slots = $("#form_nb_slot");
        port = $("#form_port");
        jitter = $("#form_jitter");
    });

    //Modification du texte
    $("div#content").on('input', '#serv_start input', function(){
        serv_path = $("#serv_path");
        nb_slots = $("#form_nb_slot");
        port = $("#form_port");
        jitter = $("#form_jitter");

        check_slots();
        check_port();
        check_jitter();

        $("p#command").text(serv_path.val()+"/./rx -i "+ nb_slots.val() + " -j " + jitter.val() + " -p " + port.val());
    });
    function check_slots() {
        if(!Math.floor(nb_slots.val()) === nb_slots.val() || !$.isNumeric(nb_slots.val())) {
            nb_slots.css({
                borderColor : 'red',
                color : 'red'
            });
            return 0;
        }
        else {
            nb_slots.css({
                borderColor : '#595959',
                color : 'black'
            });
            return 1;
        }
    }
    //Verif Port
    function check_port() {
        if(!Math.floor(port.val()) === port.val() || !$.isNumeric(port.val()) || port.val() <= 0) {
            port.css({
                borderColor : 'red',
                color : 'red'
            });
            return 0;
        }
        else {
            port.css({
                borderColor : '#595959',
                color : 'black'
            });
            return 1;
        }
    }

    //Verif jitter
    function check_jitter() {
        if(!Math.floor(jitter.val()) === jitter.val() || !$.isNumeric(jitter.val()) || jitter.val() < 0) {
            jitter.css({
                borderColor : 'red',
                color : 'red'
            });
            return 0;
        }
        else {
            jitter.css({
                borderColor : '#595959',
                color : 'black'
            });
            return 1;
        }
    }
    //Demande d'envoi
    $("div#content").on('click', '#valider_serv', function(){
        event.preventDefault();

        if(check_slots() && check_port() && check_jitter()) {
            var slot_val = encodeURIComponent(nb_slots.val());
            var port_val = encodeURIComponent(port.val());
            var jitter_val = encodeURIComponent(jitter.val());
            $.ajax({
                url : '/modules/server_start.php',
                type : 'POST',
                data : 'slots='+slot_val+'&port='+port_val+'&jitter='+jitter_val,
                dataType : 'html',
                success : function(code_html, statut) {
                    $("#serverinfos").html($(code_html));
                    load_data();
                    reload = 1;
                }
            });
        }
    });




});
            
            