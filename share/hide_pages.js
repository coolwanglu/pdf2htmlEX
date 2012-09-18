/* hide all pages */
(function(){
 var s = '.b{display:none;}';
 var n = document.createElement("style");
 n.type = "text/css";
 if (n.styleSheet) {
  n.styleSheet.cssText = s;
 } else {
  n.appendChild(document.createTextNode(s));
 }
 document.getElementsByTagName("head")[0].appendChild(n);
})();
