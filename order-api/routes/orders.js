/*
 * ./routes/orders.js
 *
 * Used to place order action from IoT device
 *
 */
var express = require('express');
var app = express();
var router  = express.Router();
var bodyParser = require('body-parser');
var Busboy = require('busboy');
var _ = require('lodash');
var mongoose = require('mongoose');
var Order = require('./../models/order');

mongoose.connect('mongodb://0.0.0.0:27017/orders/');

app.use( bodyParser() );

// Retrieve all Orders : /v1/orders/
router.route('/')
  .get( function( req, res ){
    Order.find( function( err, order ){
      if( err ){
        res.send( err );
      }

      res.json( order );
    });
  });

// Retrieve all in progress orders : /vi/orders/open
router.route('/open')
  .get( function( req, res ){
    Order.find( { completed: false }, function( err, order ){
      if( err ){
        res.send( err );
      }

      res.json( order );
    });
  });

// Retrieve single Order : /v1/orders/orderId
router.route('/:orderId')
  .get( function( req, res ){
    Order.findById( req.params.orderId, function( err, order ){
      if( err ){
        res.send( err );
      }

      res.json( order );
    });
  })
  .delete( function( req, res ){
    Order.remove( { _id: req.params.orderId }, function( err, order ){
      if( err )
        res.send( err );

      res.json({ message: "Device Removed" });
    });
  });

// Create New Device : /v1/orders/new
router.route('/new')
  .post( function( req, res, next ){

    var busboy = new Busboy({ headers: req.headers });

    busboy.on('field', function(fieldname, val, fieldnameTruncated, valTruncated) {
      req.body[fieldname] = val;
    });

    busboy.on('finish', function() {
      var order = new Order();

      order.deviceName = req.body.deviceName;
      order.orderConfirmed = false;
      order.status = 0;
      order.completed = false;

      if( order.deviceName !== undefined ){
        order.save( function( err ){
          if( err )
            res.send( err )

          res.json(order._id);
        });
      }
      else {
        res.json({ message: "Nothing Created. Not enough data." });
      }
    });
    req.pipe(busboy);

  });

// Confirm Order : /v1/orders/confirm/orderId
router.route('/confirm/:orderId')
  .get( function( req, res ){
    Order.findById( req.params.orderId, function( err, order ){
      if( err ){
        res.send( err );
      }

      order.orderConfirmed = true;

      order.save( function( err ){
        if( err ){
          res.send( err );
        }

        res.json({ message: "confirmed" });
      });

      initiateOrder( req.params.orderId, order.status );

    });
  });

// Track Order Status : /v1/orders/status/orderId
router.route('/status/:orderId')
  .get( function( req, res ){
    Order.findById( req.params.orderId, function( err, order ){
      if ( err ){
        res.send( err );
      }

      res.json( order.status );
    });
  });

// Initiate the order process incrementing Status for test purposes
function initiateOrder( orderId, status ){
  // For test purposes wait 60 seconds to update status
  setTimeout( function(){
    // Increment Status
    status ++;

    // Find Order to update
    Order.findById( orderId, function( err, order ){
      if ( err ){
        console.log( err );
      }

      // If Status is less than 6 update status, save, and repeat function
      // If status is 6 or greater complete order and save
      if( status < 6 ){
        order.status = status;
        order.save();

        initiateOrder( orderId, status );
      }
      else {
        order.status = status;
        order.completed = true;
        order.save();
      }
    });
  }, 60000 );
}

module.exports = router;
