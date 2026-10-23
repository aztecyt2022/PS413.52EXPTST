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
      ["henacy.bin?v=1-sysinfo-529a4fbb76f5",500448,"529a4fbb76f59bfc87917bc8cdd4401289e06734e3d2cbfce6170d754c71c427"]
  };
})();
