// Karma configuration
// Generated on Mon Sep 29 2014 13:21:42 GMT-0700 (PDT)

module.exports = function(config) {
  config.set({

    // base path that will be used to resolve all patterns (eg. files, exclude)
    basePath: '',

    plugins: [
      'karma-chai',
      'karma-chrome-launcher',
      'karma-mocha',
      'naclbind-test',
    ],


    // frameworks to use
    // available frameworks: https://npmjs.org/browse/keyword/karma-adapter
    frameworks: ['mocha', 'chai'],


    // list of files / patterns to load in the browser
    files: [
      'src/js/naclbind.js',
      'test/integration/test_*.js',
      {pattern: 'out/test/integration/**', watched: false, included: false, served: true},
    ],


    // list of files to exclude
    exclude: [
      'test/js/test_gen.js'
    ],


    // preprocess matching files before serving them to the browser
    // available preprocessors: https://npmjs.org/browse/keyword/karma-preprocessor
    preprocessors: {
      'test/integration/test_*.js': ['naclbind-test'],
    },


    // test results reporter to use
    // possible values: 'dots', 'progress'
    // available reporters: https://npmjs.org/browse/keyword/karma-reporter
    reporters: ['progress'],


    // web server port
    port: 9876,


    // enable / disable colors in the output (reporters and logs)
    colors: true,


    // level of logging
    // possible values: config.LOG_DISABLE || config.LOG_ERROR || config.LOG_WARN || config.LOG_INFO || config.LOG_DEBUG
    logLevel: config.LOG_INFO,


    // enable / disable watching file and executing tests whenever any file changes
    autoWatch: true,


    // start these browsers
    // available browser launchers: https://npmjs.org/browse/keyword/karma-launcher
    browsers: process.env.TRAVIS ? ['ChromeTravis'] : ['ChromeWithNaCl'],

    browserNoActivityTimeout: 60000,
//    browserNoActivityTimeout: 2000,

    customLaunchers: {
      ChromeWithNaCl: {
        base: 'Chrome',
        flags: ['--enable-nacl'],
      },

      ChromeTravis: {
        base: 'Chrome',
        flags: ['--enable-nacl', '--no-sandbox'],
      },
    },


    // Continuous Integration mode
    // if true, Karma captures browsers, runs the tests and exits
    singleRun: false
  });
};
