(function(){
  "use strict";
  var query=location.search||"";
  var legacy=/(?:[?&])legacy=1(?:&|$)/.test(query);
  var henacy=/(?:[?&])henacy=1(?:&|$)/.test(query);
  var original=!legacy&&!henacy&&/(?:[?&])original=1(?:&|$)/.test(query);
  /* No VSH overlay. ?legacy=1 is the immutable HENACY rollback binary. */
  window.HEN_BUILD={
    original:original,
    legacy:legacy,
    label:original?"PS4 HEN 2.2.0":legacy?"HENACY v1 (legacy)":"DEXCore v1",
    query:original?"?original=1":legacy?"?legacy=1":"",
    payload:original?
      ["hen.bin",499776,"568d57e7c6bfff1b96fc20a4e00b9ca744aa58b135a56eeb5c66c1175acfac3e"]:
      legacy?["henacy.bin?v=1-syslogo-ea1235b846b0",500448,"ea1235b846b08434123feec9b9c8e3d16cd9447ec31e0bd115df80589aa42352"]:
      ["dexcore.bin?v=1-edb0fa619aa4",500448,"edb0fa619aa48de7858fdc1fb2bee30dd9ec6a367fdd28766707e1aac3e9bdae"]
  };
})();
