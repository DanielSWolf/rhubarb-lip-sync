// Polyfill for Object.assign
"function"!=typeof Object.assign&&(Object.assign=function(a,b){"use strict";if(null==a)throw new TypeError("Cannot convert undefined or null to object");for(var c=Object(a),d=1;d<arguments.length;d++){var e=arguments[d];if(null!=e)for(var f in e)Object.prototype.hasOwnProperty.call(e,f)&&(c[f]=e[f])}return c});

// Polyfill for Array.isArray
Array.isArray||(Array.isArray=function(r){return"[object Array]"===Object.prototype.toString.call(r)});

// Polyfill for Array.prototype.map
Array.prototype.map||(Array.prototype.map=function(r){var t,n,o;if(null==this)throw new TypeError("this is null or not defined");var e=Object(this),i=e.length>>>0;if("function"!=typeof r)throw new TypeError(r+" is not a function");for(arguments.length>1&&(t=arguments[1]),n=new Array(i),o=0;o<i;){var a,p;o in e&&(a=e[o],p=r.call(t,a,o,e),n[o]=p),o++}return n});

// Polyfill for Array.prototype.every
Array.prototype.every||(Array.prototype.every=function(r,t){"use strict";var e,n;if(null==this)throw new TypeError("this is null or not defined");var o=Object(this),i=o.length>>>0;if("function"!=typeof r)throw new TypeError;for(arguments.length>1&&(e=t),n=0;n<i;){var y;if(n in o&&(y=o[n],!r.call(e,y,n,o)))return!1;n++}return!0});

// Polyfill for Array.prototype.find
Array.prototype.find||(Array.prototype.find=function(r){if(null===this)throw new TypeError("Array.prototype.find called on null or undefined");if("function"!=typeof r)throw new TypeError("callback must be a function");for(var n=Object(this),t=n.length>>>0,o=arguments[1],e=0;e<t;e++){var f=n[e];if(r.call(o,f,e,n))return f}});

// Polyfill for Array.prototype.filter
Array.prototype.filter||(Array.prototype.filter=function(r){"use strict";if(void 0===this||null===this)throw new TypeError;var t=Object(this),e=t.length>>>0;if("function"!=typeof r)throw new TypeError;for(var i=[],o=arguments.length>=2?arguments[1]:void 0,n=0;n<e;n++)if(n in t){var f=t[n];r.call(o,f,n,t)&&i.push(f)}return i});

// Polyfill for Array.prototype.forEach
Array.prototype.forEach||(Array.prototype.forEach=function(a,b){var c,d;if(null===this)throw new TypeError(" this is null or not defined");var e=Object(this),f=e.length>>>0;if("function"!=typeof a)throw new TypeError(a+" is not a function");for(arguments.length>1&&(c=b),d=0;d<f;){var g;d in e&&(g=e[d],a.call(c,g,d,e)),d++}});

// Polyfill for Array.prototype.includes
Array.prototype.includes||(Array.prototype.includes=function(r,t){if(null==this)throw new TypeError('"this" is null or not defined');var e=Object(this),n=e.length>>>0;if(0===n)return!1;for(var i=0|t,o=Math.max(i>=0?i:n-Math.abs(i),0);o<n;){if(function(r,t){return r===t||"number"==typeof r&&"number"==typeof t&&isNaN(r)&&isNaN(t)}(e[o],r))return!0;o++}return!1});

// Polyfill for JSON
"object"!=typeof JSON&&(JSON={}),function(){"use strict";function f(a){return a<10?"0"+a:a}function this_value(){return this.valueOf()}function quote(a){return rx_escapable.lastIndex=0,rx_escapable.test(a)?'"'+a.replace(rx_escapable,function(a){var b=meta[a];return"string"==typeof b?b:"\\u"+("0000"+a.charCodeAt(0).toString(16)).slice(-4)})+'"':'"'+a+'"'}function str(a,b){var c,d,e,f,h,g=gap,i=b[a];switch(i&&"object"==typeof i&&"function"==typeof i.toJSON&&(i=i.toJSON(a)),"function"==typeof rep&&(i=rep.call(b,a,i)),typeof i){case"string":return quote(i);case"number":return isFinite(i)?String(i):"null";case"boolean":case"null":return String(i);case"object":if(!i)return"null";if(gap+=indent,h=[],"[object Array]"===Object.prototype.toString.apply(i)){for(f=i.length,c=0;c<f;c+=1)h[c]=str(c,i)||"null";return e=0===h.length?"[]":gap?"[\n"+gap+h.join(",\n"+gap)+"\n"+g+"]":"["+h.join(",")+"]",gap=g,e}if(rep&&"object"==typeof rep)for(f=rep.length,c=0;c<f;c+=1)"string"==typeof rep[c]&&(d=rep[c],(e=str(d,i))&&h.push(quote(d)+(gap?": ":":")+e));else for(d in i)Object.prototype.hasOwnProperty.call(i,d)&&(e=str(d,i))&&h.push(quote(d)+(gap?": ":":")+e);return e=0===h.length?"{}":gap?"{\n"+gap+h.join(",\n"+gap)+"\n"+g+"}":"{"+h.join(",")+"}",gap=g,e}}var rx_one=/^[\],:{}\s]*$/,rx_two=/\\(?:["\\\/bfnrt]|u[0-9a-fA-F]{4})/g,rx_three=/"[^"\\\n\r]*"|true|false|null|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?/g,rx_four=/(?:^|:|,)(?:\s*\[)+/g,rx_escapable=/[\\"\u0000-\u001f\u007f-\u009f\u00ad\u0600-\u0604\u070f\u17b4\u17b5\u200c-\u200f\u2028-\u202f\u2060-\u206f\ufeff\ufff0-\uffff]/g,rx_dangerous=/[\u0000\u00ad\u0600-\u0604\u070f\u17b4\u17b5\u200c-\u200f\u2028-\u202f\u2060-\u206f\ufeff\ufff0-\uffff]/g;"function"!=typeof Date.prototype.toJSON&&(Date.prototype.toJSON=function(){return isFinite(this.valueOf())?this.getUTCFullYear()+"-"+f(this.getUTCMonth()+1)+"-"+f(this.getUTCDate())+"T"+f(this.getUTCHours())+":"+f(this.getUTCMinutes())+":"+f(this.getUTCSeconds())+"Z":null},Boolean.prototype.toJSON=this_value,Number.prototype.toJSON=this_value,String.prototype.toJSON=this_value);var gap,indent,meta,rep;"function"!=typeof JSON.stringify&&(meta={"\b":"\\b","\t":"\\t","\n":"\\n","\f":"\\f","\r":"\\r",'"':'\\"',"\\":"\\\\"},JSON.stringify=function(a,b,c){var d;if(gap="",indent="","number"==typeof c)for(d=0;d<c;d+=1)indent+=" ";else"string"==typeof c&&(indent=c);if(rep=b,b&&"function"!=typeof b&&("object"!=typeof b||"number"!=typeof b.length))throw new Error("JSON.stringify");return str("",{"":a})}),"function"!=typeof JSON.parse&&(JSON.parse=function(text,reviver){function walk(a,b){var c,d,e=a[b];if(e&&"object"==typeof e)for(c in e)Object.prototype.hasOwnProperty.call(e,c)&&(d=walk(e,c),void 0!==d?e[c]=d:delete e[c]);return reviver.call(a,b,e)}var j;if(text=String(text),rx_dangerous.lastIndex=0,rx_dangerous.test(text)&&(text=text.replace(rx_dangerous,function(a){return"\\u"+("0000"+a.charCodeAt(0).toString(16)).slice(-4)})),rx_one.test(text.replace(rx_two,"@").replace(rx_three,"]").replace(rx_four,"")))return j=eval("("+text+")"),"function"==typeof reviver?walk({"":j},""):j;throw new SyntaxError("JSON.parse")})}();

// ExtendScript's resource strings are a pain to write.
// This function allows them to be written in JSON notation, then converts them into the required
// format.
// For instance, this string: '{ "__type__": "StaticText", "text": "Hello world" }'
// is converted to this: 'StaticText { "text": "Hello world" }'.
// This code relies on the fact that, contrary to the language specification, all major JavaScript
// implementations keep object properties in insertion order.
function createResourceString(tree) {
	var result = JSON.stringify(tree, null, 2);
	result = result.replace(/(\{\s*)"__type__":\s*"(\w+)",?\s*/g, '$2 $1');
	return result;
}

// Object containing functions to create control description trees.
// For instance, `controls.StaticText({ text: 'Hello world' })`
// returns `{ __type__: StaticText, text: 'Hello world' }`.
var controlFunctions = (function() {
	var controlTypes = [
		// Strangely, 'dialog' and 'palette' need to start with a lower-case character
		['Dialog', 'dialog'], ['Palette', 'palette'],
		'Panel', 'Group', 'TabbedPanel', 'Tab', 'Button', 'IconButton', 'Image', 'StaticText',
		'EditText', 'Checkbox', 'RadioButton', 'Progressbar', 'Slider', 'Scrollbar', 'ListBox',
		'DropDownList', 'TreeView', 'ListItem', 'FlashPlayer'
	];
	var result = {};
	controlTypes.forEach(function(type){
		var isArray = Array.isArray(type);
		var key = isArray ? type[0] : type;
		var value = isArray ? type[1] : type;
		result[key] = function(options) {
			return Object.assign({ __type__: value }, options);
		};
	});
	return result;
})();

function createDialogWindow() {
	var resourceString;
	with (controlFunctions) {
		resourceString = createResourceString(
			Dialog({
				text: 'Rhubarb Lip-Sync',
				settings: Group({
					orientation: 'column',
					alignChildren: ['left', 'top'],
					audioFile: Group({
						label: StaticText({
							text: 'Audio file:',
							// If I don't explicitly activate a control, After Effects has trouble
							// with keyboard focus, so I can't type in the text edit field below.
							active: true
						}),
						value: DropDownList()
					}),
					dialogText: Group({
						label: StaticText({ text: 'Dialog text (optional):' }),
						value: EditText({
							properties: { multiline: true },
							characters: 60,
							minimumSize: [0, 100]
						})
					}),
					mouthComp: Group({
						label: StaticText({ text: 'Mouth composition:' }),
						value: DropDownList({})
					}),
					extendedMouthShapes: Group({
						label: StaticText({ text: 'Extended mouth shapes:' }),
						g: Checkbox({ text: 'G' }),
						h: Checkbox({ text: 'H' }),
						x: Checkbox({ text: 'X' }),
					}),
					targetFolder: Group({
						label: StaticText({ text: 'Target folder:' }),
						value: DropDownList({})
					}),
					frameRate: Group({
						label: StaticText({ text: 'Frame rate:' }),
						value: EditText({ characters: 8 }),
						auto: Checkbox({ text: 'From mouth composition' })
					})
				}),
				separator: Group({ preferredSize: ['', 3] }),
				buttons: Group({
					alignment: 'right',
					animate: Button({
						properties: { name: 'ok' },
						text: 'Animate'
					}),
					cancel: Button({
						properties: { name: 'cancel' },
						text: 'Cancel'
					})
				})
			})
		);
	}

	// Create window and child controls
	var window = new Window(resourceString);
	var controls = {
		audioFile: window.settings.audioFile.value,
		dialogText: window.settings.dialogText.value,
		mouthComp: window.settings.mouthComp.value,
		mouthShapeG: window.settings.extendedMouthShapes.g,
		mouthShapeH: window.settings.extendedMouthShapes.h,
		mouthShapeX: window.settings.extendedMouthShapes.x,
		targetFolder: window.settings.targetFolder.value,
		frameRate: window.settings.frameRate.value,
		autoFrameRate: window.settings.frameRate.auto,
		animateButton: window.buttons.animate,
		cancelButton: window.buttons.cancel
	};
	
	// Align controls
	window.onShow = function() {
		// Give uniform width to all labels
		var groups = toArray(window.settings.children);
		var labelWidths = groups.map(function(group) { return group.children[0].size.width; });
		var maxLabelWidth = Math.max.apply(Math, labelWidths);
		groups.forEach(function (group) {
			group.children[0].size.width = maxLabelWidth;
		});

		// Give uniform width to inputs
		var valueWidths = groups.map(function(group) {
			return last(group.children).bounds.right - group.children[1].bounds.left;
		});
		var maxValueWidth = Math.max.apply(Math, valueWidths);
		groups.forEach(function (group) {
			var multipleControls = group.children.length > 2;
			if (!multipleControls) {
				group.children[1].size.width = maxValueWidth;
			}
		});

		window.layout.layout(true);
	};

	// Handle animation
	controls.animateButton.onClick = function() {
		// TODO: validate
		app.beginUndoGroup('Rhubarb Lip-Sync');
		window.close();
		// TODO: animate
		app.endUndoGroup();
	};
	
	// Handle cancelation
	controls.cancelButton.onClick = function() {
		window.close();
	};

	return window;
}

createDialogWindow().show();
