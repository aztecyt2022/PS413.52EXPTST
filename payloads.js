(function(){
  "use strict";
  var query=location.search||"";
  var original=/(?:[?&])original=1(?:&|$)/.test(query);
  var manual=!original&&/(?:[?&])overlay=1(?:&|$)/.test(query);
  window.HEN_BUILD={
    original:original,
    manualOnly:manual,
    label:original?"PS4 HEN 2.2.0":"PROHEN 2.2.8",
    query:original?"?original=1":manual?"?overlay=1":"",
    payload:original?
      ["hen.bin",499776,"568d57e7c6bfff1b96fc20a4e00b9ca744aa58b135a56eeb5c66c1175acfac3e"]:
      ["prohen.bin?v=2.2.8-build4",587904,"f04082626ee6bae2484e72d9a392f24ab5bcfe007e3793a464ff4ab058b54f4f"]
  };
})();
