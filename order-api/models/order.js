/*
 * models/order.js
 */
var mongoose = require('mongoose');
var Schema = mongoose.Schema;

var OrderSchema = new Schema({
  deviceName: String,
  orderConfirmed: Boolean,
  status: Number,
  completed: Boolean
});

module.exports = mongoose.model('Order', OrderSchema);
