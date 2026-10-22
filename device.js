(function(){
  "use strict";
  function firmware(ua){
    var match=/\bPlayStation\s+4[\/ ](\d+)\.(\d+)(?=[); \t]|$)/.exec(ua||"");
    if(!match)return null;
    var minor=parseInt(match[2],16).toString(16);
    while(minor.length<2)minor="0"+minor;
    return match[1]+"."+minor;
  }
  var current=firmware(navigator.userAgent);
  window.HEN_DEVICE={firmware:current,supported:current==="13.52",parse:firmware};
})();
