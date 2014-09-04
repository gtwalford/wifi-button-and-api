Node server runs on port 3000

To start Node server and watch requests :
- `DEBUG=order-api ./bin/www` - Great for debuggin but not reliable if left persistently running on server

To start Node server in background with Forever :
- `DEBUG=order-api forever start ./bin/www` - Does not show any logs but is reliable to leave running persistently

To Stop Forever
- `forever stop 0` or `forever stopall`

To Restart Forever
- `forever restart 0` or `forever restartall`
