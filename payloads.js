(function(){
  "use strict";
  var query=location.search||"";
  var henacy=/(?:[?&])henacy=1(?:&|$)/.test(query);
  var original=!henacy&&/(?:[?&])original=1(?:&|$)/.test(query);
  var vsh=!henacy&&!original&&/(?:[?&])vsh=1(?:&|$)/.test(query);
  window.HEN_BUILD={
    original:original,
    vsh:vsh,
    label:original?"PS4 HEN 2.2.0":vsh?"HENACY v1 — VSH test":"HENACY v1",
    query:original?"?original=1":vsh?"?vsh=1":"",
    payload:original?
      ["hen.bin",499776,"568d57e7c6bfff1b96fc20a4e00b9ca744aa58b135a56eeb5c66c1175acfac3e"]:
      vsh?["../builds/vsh/henacy-vsh.bin?v=0.1-fabe656438e3",500448,"fabe656438e3fec89d490ecaf0585f829286eef0a9f119954a6e5c677288537f"]:
      ["henacy.bin?v=1-syslogo-ea1235b846b0",500448,"ea1235b846b08434123feec9b9c8e3d16cd9447ec31e0bd115df80589aa42352"]
  };
})();
