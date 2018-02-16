<!DOCTYPE html>
<!--
To change this license header, choose License Headers in Project Properties.
To change this template file, choose Tools | Templates
and open the template in the editor.
-->
<html>
    <head>
        <meta charset="UTF-8">
        <link rel="stylesheet" href="public/css/style.css" />
        <title>Administration serveur TRX</title>
    </head>
    <body>
        <?php
        require("classes/Socket.class.php");
        require("classes/Server.class.php");

        ?>
        <div id="header">
            <h1>Administration TRX</h1>
            <p>Page d'administration du serveur TRX local</p>
        </div>
        <!-- <div id="menu">
            <a href="/server/" >Serveur</a> | 
        </div> -->
        <div id="content">
            <div id="server">
                <h1>Infos Serveur</h1>
                <img src="public/img/loading.gif" alt="loading" />
            </div>
        </div>
        <div id="bottom">
            <p>Copyright etc...</p>
        </div>
        <script src="https://ajax.googleapis.com/ajax/libs/jquery/1.7.2/jquery.min.js"></script>
        <script type="text/javascript" src="public/js/main.js"></script>
    </body>
</html>