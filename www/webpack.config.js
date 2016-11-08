// var failPlugin = require('webpack-fail-plugin');
var ExtractTextPlugin = require('extract-text-webpack-plugin');
var LiveReloadPlugin = require('webpack-livereload-plugin');

module.exports = {
     entry: './src/script.js',
     resolve: {
        // Add '.ts' and '.tsx' as a resolvable extension.
        extensions: [".webpack.js", ".web.js", ".js"]
    },
     output: {
         path: './bin',
         filename: 'app.bundle.js'
     },
     module: {
        loaders: [
            { test: /\.css$/, loader: ExtractTextPlugin.extract("style-loader", "css-loader") },
            { test: /\.(jpg|png)$/, loader: 'file' },
        ]
    },
     plugins: [
        new ExtractTextPlugin("app.css"),
        new LiveReloadPlugin()
     ]

 };