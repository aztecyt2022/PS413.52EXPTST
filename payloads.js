(function(){
  "use strict";
  var query=location.search||"";
  var original=/(?:[?&])original=1(?:&|$)/.test(query);
  var manual=!original&&/(?:[?&])overlay=1(?:&|$)/.test(query);
  window.HEN_BUILD={
    original:original,
    manualOnly:manual,
    label:original?"PS4 HEN 2.2.0":"PROHEN 2.2.0",
    query:original?"?original=1":manual?"?overlay=1":"",
    payload:original?
      ["hen.bin",499776,"568d57e7c6bfff1b96fc20a4e00b9ca744aa58b135a56eeb5c66c1175acfac3e"]:
      ["prohen.bin",517920,"663761e7dd8e7848c41d416afa5f3021151a4ff340438bc9cc358adaaff8ca15"]
  };
})();
