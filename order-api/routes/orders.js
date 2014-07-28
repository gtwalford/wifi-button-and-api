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

// Retrieve all in progress orders : /v1/orders/open
router.route('/open')
  .get( function( req, res ){
    Order.find( { completed: false }, function( err, order ){
      if( err ){
        res.send( err );
      }

      res.render( 'orderWatch', { orders: order } );
    });
  });

// Clear all orders that have not been completed : /v1/orders/clear
router.route('/clear')
  .delete( function( req, res ){
    Order.remove( { completed: false }, function( err, order ){
      if( err ){
        res.send( err );
      }

      res.json({ message: "Removed orders that have not been confirmed" });
    });
  });

// Single Order : /v1/orders/orderId
// Get to retrieve
// Delete to remove
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

// To create a new order posting content from header : /v1/orders/new
router.route('/new')
  .post( function( req, res ){

    var order = new Order();
    order.deviceName = req.body.deviceName;
    order.orderConfirmed = false;
    order.status = 0;
    order.completed = false;
    if( order.deviceName !== undefined ){
      order.save( function( err ){
        if( err )
          res.send( err )

        res.json( '{'+ order._id.toString() +'}' );
      });
    }
  })

// Create new order posting content through the url : /v1/orders/new/deviceName
router.route('/new/:deviceName')
  .post( function( req, res ){

    var order = new Order();
    order.deviceName = req.params.deviceName;
    order.orderConfirmed = false;
    order.status = 0;
    order.completed = false;
    if( order.deviceName !== undefined ){
      order.save( function( err ){
        if( err )
          res.send( err )

        console.log('New Order Created');
        console.log('Confirmation number = ' + order._id);
        res.json( '{'+ order._id.toString() +'}' );
      });
    }
  });

// Confirm Order : /v1/orders/confirm/orderId
router.route('/confirm/:orderId')
  .get( function( req, res ){
    Order.findById( req.params.orderId, function( err, order ){
      if( err ){
        res.send( err );
      }

      order.orderConfirmed = true;
      order.status++;

      order.save( function( err ){
        if( err ){
          res.send( err );
        }

        res.json( "{confirmed}" );
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

      res.json( '{'+order.status.toString()+'}' );
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
  }, 10000 );
}

module.exports = router;
