(function(){
  "use strict";
  var query=location.search||"";
  var henacy=/(?:[?&])henacy=1(?:&|$)/.test(query);
  var original=!henacy&&/(?:[?&])original=1(?:&|$)/.test(query);
  window.HEN_BUILD={
    original:original,
    label:original?"PS4 HEN 2.2.0":"HENACY v1",
    query:original?"?original=1":"",
    payload:original?
      ["hen.bin",499776,"568d57e7c6bfff1b96fc20a4e00b9ca744aa58b135a56eeb5c66c1175acfac3e"]:
      ["henacy.bin?v=1-icon-7163545ede8f",500448,"7163545ede8f6990539f73a22f1dc13f1ecd1f9019e7978c8d91a979c197e752"]
  };
})();
