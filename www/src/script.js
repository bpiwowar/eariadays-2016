"use strict";

var $ = require('jquery');
window.jQuery = $;

require('jquery-ui');
require('jquery-ui/themes/base/core.css');
require('jquery-ui/themes/base/theme.css');

require('jquery.tocify');
require('jquery.tocify/src/stylesheets/jquery.tocify.css');

$(window).on('load', function() {
	$("#toc").tocify({
		selectors: "h1,h2,h3,h4"
	});
});

// Highlight
var hljs = require('highlight.js');
require('highlight.js/styles/agate.css');
hljs.initHighlightingOnLoad();
