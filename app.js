// Start sails and pass it command line arguments
require('sails').lift(require('optimist').argv);

// Fill in with your personal username and API key
// or, use this public demo account
var plotly = require('plotly')('Node.js-Demo-Account','dvlqkmw0zm');

var data = [
	    {
		x: ["giraffes", "orangutans", "monkeys"], 
		y: [20, 14, 23], 
		type: "bar"
	    }
	    ];

var graph_options = {filename: "basic-bar", fileopt: "overwrite"}
    plotly.plot(data, graph_options, function (err, msg) {
	    console.log(msg);
	});
