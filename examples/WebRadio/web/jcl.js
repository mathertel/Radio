// jcl.js: JavaScript Common behaviours Library
// -----
// Behaviour loading and DataConnections for AJAX Controls
// Copyright (c) by Matthias Hertel, http://www.mathertel.de
// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
// More information on: http://ajaxaspects.blogspot.com/ and http://ajaxaspekte.blogspot.com/
// -----
// 12.08.2005 created
// 31.08.2005 jcl object used instead of global methods and objects
// 04.09.2005 GetPropValue added.
// 15.09.2005 CloneObject added.
// 27.09.2005 nosubmit attribute without forms bug fixed.
// 29.12.2005 FindBehaviourElement added.
// 02.04.2005 term method added to release bound html objects.
// 07.05.2006 controlsPath added.
// 09.05.2006 Raise only changed values.
// 09.05.2006 Load() and RaiseAll() added to support default-values on page start.
// 03.06.2006 binding to the document enabled for FF.
// 13.08.2006 classname functions added.
// 16.09.2006 bind() function added to the Function.prototype.
//            The "on___" functions that are automatically attached to events
//            are now always executed in the context of the bound object.
// 24.11.2006 DataConnections without a name are ignored now.
// 09.01.2007 allow Array.prototype.extensions (prototype.js)
// 06.06.2007 including an Open Ajax hub specification compatible implementation
// 11.07.2007 more robust classname functions.
// 15.08.2007 afterinit method support for JavaScript Behviors. This method is called after all components
//            on a page have been initialized.
// 22.08.2007 jcl.getCookie and jcl.setCookie added.
// 15.09.2007 BuildFullEventname added.
// 07.10.2007 Firefox bugs fixed.
// 11.11.2007 isIE removed: use jcl.isIE instead.
// 20.11.2007 DataConnections a.k.a. Page Properties finally removed to favour the OpenAjax.hub
// 12.12.2007 adding EventNameSpace
// 12.01.2007 initstate method support for JavaScript Behviors.
// 02.01.2009 adding new nls support
// 05.12.2009 fixed getCookie
// 01.10.2010 IE9 compatibility avoiding browser detection.
// 02.11.2016 radio version.

// initialization sequence: init(), initstate(), afterinit()

// -- OpenAjax hub --

if (typeof(window.OpenAjax) == "undefined") {
  // setup the OpenAjax framework - hub
  window.OpenAjax = {};
} // if

if (typeof(OpenAjax.hub) == "undefined") {

  // a hub implementation
  OpenAjax.hub = {
    implementer: "http://www.mathertel.de/OpenAjax",
    implVersion: "0.4",
    specVersion: "0.5",
    implExtraData: {},

    // ----- library management -----

    // the list of libraries that have registered
    libraries: {},

    // Registers an Ajax library with the OpenAjax Hub. 
    registerLibrary: function(p, u, v, e) {
      var entry = { prefix: p, namespaceURI: u, version: v, extraData: e };
      this.libraries[p] = entry;
      this.publish("org.openajax.hub.registerLibrary", entry);
    },

    // Unregisters an Ajax library with the OpenAjax Hub.
    unregisterLibrary: function(p) {
      var entry = this.libraries[p];
      this.publish("org.openajax.hub.unregisterLibrary", entry);
      if (entry)
        this.libraries[p] = null;
    },

    // ----- event management -----

    _regs: {},
    _regsId: 0,

    /// name, callback, scope, data, filter
    subscribe: function(n, c, s, d, f) {
      var h = this._regsId;

      s = s || window;

      // treating upper/lowercase equal is not clearly defined, but true with domain names.
      var rn = n.toLocaleLowerCase();

      // build a regexp pattern that will match the event names
      rn = rn.replace(/\*\*$/, "\S{0,}").replace(/\./g, "\\.").replace(/\*/g, "[^.]*");

      var entry = { id: h, n: rn, c: c, s: s, d: d, f: f };
      this._regs[h] = entry;

      this._regsId++;
      return (h);
    }, // subscribe


    unsubscribe: function(h) {
      this._regs[h] = null;
    }, // unsubscribe


    publish: function(n, data) {
        if ((n) && (n.length > 0)) {
          n = n.toLocaleLowerCase();
          for (var h in this._regs) {
            var r = this._regs[h];
            if (r && (n.match(r.n))) {
              var ff = r.f;
              if (typeof(ff) == "string") ff = r.s[ff];
              var fc = r.c;
              if (typeof(fc) == "string") fc = r.s[fc];
              if ((!ff) || (ff.call(r.s, n, data, r.d)))
                fc.call(r.s, n, data, r.d);
            } // if
          } // for
        } // if
      } // publish

  }; // OpenAjax.hub
  OpenAjax.hub.registerLibrary("aoa", "http://www.mathertel.de/OpenAjax", "0.4", {});

} // if (! hub)

OpenAjax.hub.registerLibrary("jcl", "http://www.mathertel.de/Behavior", "1.2", {});

// -- Javascript Control Library (behaviors) --

if (typeof(window.jcl) == "undefined") {
  // setup the jcl root object.
  window.jcl = {};
} // if

// Detect InternetExplorer for some specific implementation differences.
jcl.isIE = (window.navigator.userAgent.indexOf("MSIE") > 0);

/// A list with all objects that are attached to any behaviour
jcl.List = [];

// attach events, methods and default-values to a html object (using the english spelling)
jcl.LoadBehaviour = function(obj, behaviour) {
  if ((obj) && (obj.constructor == String))
    obj = document.getElementById(obj);

  if (obj == null) {
    alert("LoadBehaviour: obj argument is missing.");
  } else if (behaviour == null) {
    alert("LoadBehaviour: behaviour argument is missing.");

  } else {
    if (behaviour.inheritFrom) {
      jcl.LoadBehaviour(obj, behaviour.inheritFrom);
      jcl.List.pop();
    }

    if (obj.attributes) { // IE9 compatible
      // copy all new attributes to jcl.properties
      for (var n = 0; n < obj.attributes.length; n++)
        if (obj[obj.attributes[n].name] == null)
          obj[obj.attributes[n].name] = obj.attributes[n].value;
    } // if

    for (var p in behaviour) {
      if (p.substr(0, 2) == "on") {
        jcl.AttachEvent(obj, p, behaviour[p].bind(obj));

      } else if ((behaviour[p] == null) || (behaviour[p].constructor != Function)) {
        // set default-value
        if (obj[p] == null)
          obj[p] = behaviour[p];

      } else {
        // attach method
        obj[p] = behaviour[p];
      } // if
    } // for

    obj._attachedBehaviour = behaviour;
  } // if
  if (obj)
    jcl.List.push(obj);
}; // LoadBehaviour


/// Find the parent node of a given object that has the behavior attached.
jcl.FindBehaviourElement = function(obj, behaviourDef) {
  while ((obj) && (obj._attachedBehaviour != behaviourDef))
    obj = obj.parentNode;
  return (obj);
}; // FindBehaviourElement


/// Find the child elements with a given className contained by obj.
jcl.getElementsByClassName = function(obj, cName) {
  var ret = new Array();
  var allNodes = obj.getElementsByTagName("*");
  for (var n = 0; n < allNodes.length; n++) {
    if (allNodes[n].className == cName)
      ret.push(allNodes[n]);
  }
  return (ret);
}; // getElementsByClassName


/// Find the child elements with a given name contained by obj.
jcl.getElementsByName = function(obj, cName) {
  var ret = new Array();
  var allNodes = obj.getElementsByTagName("*");
  for (var n = 0; n < allNodes.length; n++) {
    if (allNodes[n].name == cName)
      ret.push(allNodes[n]);
  }
  return (ret);
}; // getElementsByName


// cross browser compatible helper to register for events
jcl.AttachEvent = function(obj, eventname, handler) {
  if (obj.addEventListener) { // IE9 compatible
    obj.addEventListener(eventname.substr(2), handler, false);
  } else {
    obj.attachEvent(eventname, handler);
  } // if
}; // AttachEvent


// cross browser compatible helper to register for events
jcl.DetachEvent = function(obj, eventname, handler) {
  if (obj.removeEventListener) { // IE9 compatible
    obj.removeEventListener(eventname.substr(2), handler, false);
  } else {
    obj.detachEvent(eventname, handler);
  } // if
}; // DetachEvent


/// Create a duplicate of a given JavaScript Object.
/// References are not duplicated !
jcl.CloneObject = function(srcObject) {
  var tarObject = new Object();
  for (var p in srcObject)
    tarObject[p] = srcObject[p];
  return (tarObject);
}; // CloneObject


// calculate the absolute position of an html element
jcl.absolutePosition = function(obj) {
  var pos = null;

  if (obj) {
    pos = new Object();
    pos.top = obj.offsetTop;
    pos.left = obj.offsetLeft;
    pos.width = obj.offsetWidth;
    pos.height = obj.offsetHeight;

    obj = obj.offsetParent;
    while (obj) {
      pos.top += obj.offsetTop;
      pos.left += obj.offsetLeft;
      obj = obj.offsetParent;
    } // while
  }
  return (pos);
}; // _absolutePosition


/// When an object publishes or subscribes events it is possible to define the complete eventname
/// by a local eventname and a eventnamespace of a surrounding object.
jcl.BuildFullEventname = function(obj) {
  var en = null;

  // find the local event name on the object itself.
  if (!obj) {
    return (null);
  } else if ((obj.eventname) && (obj.eventname.length > 0)) {
    en = obj.eventname;
  } else if ((obj.attributes["eventname"]) && (obj.attributes["eventname"].value.length > 0)) {
    en = obj.attributes["eventname"].value;
  } // if

  // search the event namespace if not present in the local eventname.
  if ((en) && (en.indexOf('.') < 0)) {
    while ((obj) && (!obj.eventnamespace) && ((obj.attributes) && (!obj.attributes["eventnamespace"])))
      obj = obj.parentNode;
    if (obj == document) {
      en = "jcl." + en; // default namespace, if none is specified.
    } else if ((obj) && (obj.eventnamespace)) {
      en = obj.eventnamespace + "." + en;
    } else if ((obj) && (obj.attributes["eventnamespace"])) {
      en = obj.attributes["eventnamespace"].value + "." + en;
    } // if
  } // if
  return (en);
}; // BuildFullEventname


/// Return the local part of a full qualified eventname.
jcl.LocalEventName = function(evn) {
  var idx;
  if (evn) {
    idx = evn.lastIndexOf('.');
    if (idx >= 0)
      evn = evn.substr(idx + 1);
  } // if
  return (evn);
}; // LocalEventName


/// Return the eventnamesapce of a full qualified eventname.
jcl.EventNameSpace = function(evn) {
  var idx;
  if (evn) {
    idx = evn.lastIndexOf('.');
    if (idx >= 0)
      evn = evn.substr(0, idx);
    else
      evn = null;
  } // if
  return (evn);
}; // EventNameSpace


// find a relative link to the controls folder containing jcl.js
jcl.GetControlsPath = function() {
  var path = "../controls/";
  var s;
  for (var n in document.scripts) {
    s = String(document.scripts[n].src);
    if ((s) && (s.length >= 6) && (s.substr(s.length - 6).toLowerCase() == "jcl.js"))
      path = s.substr(0, s.length - 6);
  } // for
  return (path);
}; // GetControlsPath


// init all objects when the page is loaded
jcl.onload = function(evt) {
  var obj, c;
  evt = evt || window.event;

  // initialize all 
  for (c in jcl.List) {
    obj = jcl.List[c];
    if ((obj) && (obj.init))
      obj.init();
  } // for

  for (c in jcl.List) {
    obj = jcl.List[c];
    if ((obj) && (obj.initstate))
      obj.initstate();
  } // for

  for (c in jcl.List) {
    obj = jcl.List[c];
    if ((obj) && (obj.afterinit))
      obj.afterinit();
  } // for
}; // onload


// init all objects when the page is loaded
jcl.onunload = function(evt) {
  evt = evt || window.event;

  for (var n in jcl.List) {
    var obj = jcl.List[n];
    if ((obj) && (obj.term))
      obj.term();
  } // for
}; // onunload


// allow non-submitting input elements
jcl.onkeypress = function(evt) {
  evt = evt || window.event;

  if (evt.keyCode == 13) {
    var obj = document.activeElement;

    while ((obj) && (obj.nosubmit == null))
      obj = obj.parentNode;

    if ((obj) && ((obj.nosubmit == true) || (obj.nosubmit.toLowerCase() == "true"))) {
      // cancle ENTER / RETURN
      evt.cancelBubble = true;
      evt.returnValue = false;
    } // if
  } // if              
}; // onkeypress


// --- cookie helper methods ---
// from http: //www.elated.com/articles/javascript-and-cookies/
jcl.getCookie = function(aName) {
  var results = document.cookie.match('(^;) ?' + aName + '=([^;]*)(;$)');
  if (results)
    return (unescape(results[2]));
  else
    return (null);
}; // _getCookie


jcl.setCookie = function(aName, value, path, expire) {
  if ((path == null) || (path == "")) {
    // use the current folder from the url for the cookie to avoid conflicts
    path = String(window.location.href).split('/');
    path = '/' + path.slice(3, path.length - 1).join('/');
  } // if

  if (expire) {
    var today = new Date();
    expire = parseInt(expire, 10) * 1000 * 60 * 60 * 24;
    expire = new Date(today.getTime() + expire);
  } else {
    expire = null;
  } // if

  window.document.cookie = aName + "=" + escape(value) +
    ((path) ? ';path=' + path : "") +
    ((expire) ? ";expires=" + expire.toGMTString() : "");
}; // setCookie


/// Call a function before next repaint. The function is triggered only once.
/// See http://www.w3.org/TR/animation-timing/
function throttle(func) {
  var isStarted = false;

  if (typeof func != 'function')
    throw new Error("throttle: not a function");

  return function() {
    var context = this;

    if (!isStarted) {
      isStarted = true;
      window.requestAnimationFrame(function() {
        func.call(context);
        isStarted = false;
      });
    } // if
  }; // function()
} // throttle


jcl.init = function() {
  jcl.AttachEvent(window, "onload", jcl.onload);
  jcl.AttachEvent(window, "onunload", jcl.onunload);
  jcl.AttachEvent(document, "onkeypress", jcl.onkeypress);
}; // init

// document.jcl_isinit (is not declared!) will be set to true to detect multiple jcl includes.
if (document.jcl_isinit)
  alert("multiple jcl includes detected !");
document.jcl_isinit = true;

jcl.init();

// ----- make FF more IE compatible -----
if (!jcl.isIE) {

  // ----- HTML objects -----

  HTMLElement.prototype.__defineGetter__("innerText", function() { return (this.textContent); });
  HTMLElement.prototype.__defineSetter__("innerText", function(txt) { this.textContent = txt; });

  HTMLElement.prototype.__defineGetter__("XMLDocument", function() {
    return ((new DOMParser()).parseFromString(this.innerHTML, "text/xml"));
  });


  // ----- Event objects -----

  // enable using evt.cancelBubble=true in Mozilla/Firefox
  Event.prototype.__defineSetter__("cancelBubble", function(b) {
    if (b) this.stopPropagation();
  });

  // enable using evt.returnValue=false in Mozilla/Firefox
  Event.prototype.__defineSetter__("returnValue", function(b) {
    if (!b) this.preventDefault();
  });


  // ----- XML objects -----

  // select the first node that matches the XPath expression
  // xPath: the XPath expression to use
  if (!XMLDocument.selectSingleNode) {
    XMLDocument.prototype.selectSingleNode = function(xPath) {
      var doc = this;
      if (doc.nodeType != 9)
        doc = doc.ownerDocument;
      if (doc.nsResolver == null) doc.nsResolver = function(prefix) { return (null); };
      var node = doc.evaluate(xPath, this, doc.nsResolver, XPathResult.ANY_UNORDERED_NODE_TYPE, null);
      if (node) node = node.singleNodeValue;
      return (node);
    }; // selectSingleNode
  } // if

  // select the first node that matches the XPath expression
  // xPath: the XPath expression to use
  if (!Node.selectSingleNode) {
    Node.prototype.selectSingleNode = function(xPath) {
      var doc = this;
      if (doc.nodeType != 9)
        doc = doc.ownerDocument;
      if (doc.nsResolver == null) doc.nsResolver = function(prefix) { return (null); };
      var node = doc.evaluate(xPath, this, doc.nsResolver, XPathResult.ANY_UNORDERED_NODE_TYPE, null);
      if (node) node = node.singleNodeValue;
      return (node);
    }; // selectSingleNode
  } // if


  Node.prototype.__defineGetter__("text", function() {
    return (this.textContent);
  }); // text

}


// ----- Enable an easy attaching methods to events -----
// see http://digital-web.com/articles/scope_in_javascript/

Function.prototype.bind = function(obj) {
  var method = this,
    temp = function() {
      return method.apply(obj, arguments);
    };
  return (temp);
}; // Function.prototype.bind

// ----- End -----