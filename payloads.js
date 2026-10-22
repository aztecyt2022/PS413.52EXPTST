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
      ["prohen.bin",517440,"ac645022bf01368f59ebde7bdd17ff8a0c6e2790d12acb72d37ee32d5ccd522a"]
  };
})();
