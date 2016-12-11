var express = require('express');
var winston = require('winston');
var expressWinston = require('express-winston');
var _ = require('lodash');
var request = require('request');
var bodyParser = require('body-parser');
var proxy = require('express-http-proxy');


var DYNAMO_ADDR = 'http://localhost:8093';
var KINESIS_ADDR = 'http://localhost:8094';

var makeApp = function() {
    var app = express();
    app.use(expressWinston.logger({
        transports: [
            new winston.transports.Console({
                json: false,
                colorize: true
            })
        ],
        meta: true,
        msg: "HTTP {{req.method}} {{req.url}}",
        expressFormat: true,
        colorize: true
    }));
    app.use('/', proxy('http://localhost:8093', {
        filter: function(req, res) {
            var amzTarget = req.get('x-amz-target');
            if (amzTarget && amzTarget.includes('Dynamo')) {
                return true;
            }
            return false;
        }
    }));
    app.use('/', proxy('http://localhost:8094', {
        filter: function(req, res) {
            var amzTarget = req.get('x-amz-target');
            if (amzTarget && amzTarget.includes('Kinesis')) {
                return true;
            }
            return false;
        }
    }));
    return app;
};

var main = function() {
    app = makeApp();
    var port = 80;
    app.listen(port, function() {
        winston.info("Listening on port: ", port);
    });
};

main();
