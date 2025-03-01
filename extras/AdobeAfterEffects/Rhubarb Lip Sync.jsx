var scriptName = 'Rhubarb for After Effects';

////////////////////////////////////////////////////////////////////////////////////////////////////
// Polyfills
////////////////////////////////////////////////////////////////////////////////////////////////////

// prettier-ignore
function loadPolyfills() {
  // Object.assign
  Object.assign||(Object.assign=function(a,b){"use strict";if(null==a)throw new TypeError("Cannot convert undefined or null to object");for(var c=Object(a),d=1;d<arguments.length;d++){var e=arguments[d];if(null!=e)for(var f in e)Object.prototype.hasOwnProperty.call(e,f)&&(c[f]=e[f])}return c});
  // Object.fromEntries
  Object.fromEntries||(Object.fromEntries=function(e){var r={};for(var i=0;i<e.length;i++){r[e[i][0]]=e[i][1]};return r})
  // Array.isArray
  Array.isArray||(Array.isArray=function(r){return"[object Array]"===Object.prototype.toString.call(r)});
  // Array.prototype.map
  Array.prototype.map||(Array.prototype.map=function(r){var t,n,o;if(null==this)throw new TypeError("this is null or not defined");var e=Object(this),i=e.length>>>0;if("function"!=typeof r)throw new TypeError(r+" is not a function");for(arguments.length>1&&(t=arguments[1]),n=new Array(i),o=0;o<i;){var a,p;o in e&&(a=e[o],p=r.call(t,a,o,e),n[o]=p),o++}return n});
  // Array.prototype.every
  Array.prototype.every||(Array.prototype.every=function(r,t){"use strict";var e,n;if(null==this)throw new TypeError("this is null or not defined");var o=Object(this),i=o.length>>>0;if("function"!=typeof r)throw new TypeError;for(arguments.length>1&&(e=t),n=0;n<i;){var y;if(n in o&&(y=o[n],!r.call(e,y,n,o)))return!1;n++}return!0});
  // Array.prototype.find
  Array.prototype.find||(Array.prototype.find=function(r){if(null===this)throw new TypeError("Array.prototype.find called on null or undefined");if("function"!=typeof r)throw new TypeError("callback must be a function");for(var n=Object(this),t=n.length>>>0,o=arguments[1],e=0;e<t;e++){var f=n[e];if(r.call(o,f,e,n))return f}});
  // Array.prototype.filter
  Array.prototype.filter||(Array.prototype.filter=function(r){"use strict";if(void 0===this||null===this)throw new TypeError;var t=Object(this),e=t.length>>>0;if("function"!=typeof r)throw new TypeError;for(var i=[],o=arguments.length>=2?arguments[1]:void 0,n=0;n<e;n++)if(n in t){var f=t[n];r.call(o,f,n,t)&&i.push(f)}return i});
  // Array.prototype.forEach
  Array.prototype.forEach||(Array.prototype.forEach=function(a,b){var c,d;if(null===this)throw new TypeError(" this is null or not defined");var e=Object(this),f=e.length>>>0;if("function"!=typeof a)throw new TypeError(a+" is not a function");for(arguments.length>1&&(c=b),d=0;d<f;){var g;d in e&&(g=e[d],a.call(c,g,d,e)),d++}});
  // Array.prototype.includes
  Array.prototype.includes||(Array.prototype.includes=function(r,t){if(null==this)throw new TypeError('"this" is null or not defined');var e=Object(this),n=e.length>>>0;if(0===n)return!1;for(var i=0|t,o=Math.max(i>=0?i:n-Math.abs(i),0);o<n;){if(function(r,t){return r===t||"number"==typeof r&&"number"==typeof t&&isNaN(r)&&isNaN(t)}(e[o],r))return!0;o++}return!1});
  // Array.prototype.indexOf
  Array.prototype.indexOf||(Array.prototype.indexOf=function(r,t){var n;if(null==this)throw new TypeError('"this" is null or not defined');var e=Object(this),i=e.length>>>0;if(0===i)return-1;var o=0|t;if(o>=i)return-1;for(n=Math.max(o>=0?o:i-Math.abs(o),0);n<i;){if(n in e&&e[n]===r)return n;n++}return-1});
  // Array.prototype.some
  Array.prototype.some||(Array.prototype.some=function(r){"use strict";if(null==this)throw new TypeError("Array.prototype.some called on null or undefined");if("function"!=typeof r)throw new TypeError;for(var e=Object(this),o=e.length>>>0,t=arguments.length>=2?arguments[1]:void 0,n=0;n<o;n++)if(n in e&&r.call(t,e[n],n,e))return!0;return!1});
  // String.prototype.padStart
  String.prototype.padStart||(String.prototype.padStart=function(l,p){p=String(p||" ");l-=this.length;if(l>p.length)p+=p.repeat(Math.ceil(l/p.length));return p.slice(0,l)+String(this)});
  // String.prototype.trim
  String.prototype.trim||(String.prototype.trim=function(){return this.replace(/^[\s\uFEFF\xA0]+|[\s\uFEFF\xA0]+$/g,"")});
  // JSON
  "object"!=typeof JSON&&(JSON={}),function(){"use strict";function f(a){return a<10?"0"+a:a}function this_value(){return this.valueOf()}function quote(a){return rx_escapable.lastIndex=0,rx_escapable.test(a)?'"'+a.replace(rx_escapable,function(a){var b=meta[a];return"string"==typeof b?b:"\\u"+("0000"+a.charCodeAt(0).toString(16)).slice(-4)})+'"':'"'+a+'"'}function str(a,b){var c,d,e,f,h,g=gap,i=b[a];switch(i&&"object"==typeof i&&"function"==typeof i.toJSON&&(i=i.toJSON(a)),"function"==typeof rep&&(i=rep.call(b,a,i)),typeof i){case"string":return quote(i);case"number":return isFinite(i)?String(i):"null";case"boolean":case"null":return String(i);case"object":if(!i)return"null";if(gap+=indent,h=[],"[object Array]"===Object.prototype.toString.apply(i)){for(f=i.length,c=0;c<f;c+=1)h[c]=str(c,i)||"null";return e=0===h.length?"[]":gap?"[\n"+gap+h.join(",\n"+gap)+"\n"+g+"]":"["+h.join(",")+"]",gap=g,e}if(rep&&"object"==typeof rep)for(f=rep.length,c=0;c<f;c+=1)"string"==typeof rep[c]&&(d=rep[c],(e=str(d,i))&&h.push(quote(d)+(gap?": ":":")+e));else for(d in i)Object.prototype.hasOwnProperty.call(i,d)&&(e=str(d,i))&&h.push(quote(d)+(gap?": ":":")+e);return e=0===h.length?"{}":gap?"{\n"+gap+h.join(",\n"+gap)+"\n"+g+"}":"{"+h.join(",")+"}",gap=g,e}}var rx_one=/^[\],:{}\s]*$/,rx_two=/\\(?:["\\\/bfnrt]|u[0-9a-fA-F]{4})/g,rx_three=/"[^"\\\n\r]*"|true|false|null|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?/g,rx_four=/(?:^|:|,)(?:\s*\[)+/g,rx_escapable=/[\\"\u0000-\u001f\u007f-\u009f\u00ad\u0600-\u0604\u070f\u17b4\u17b5\u200c-\u200f\u2028-\u202f\u2060-\u206f\ufeff\ufff0-\uffff]/g,rx_dangerous=/[\u0000\u00ad\u0600-\u0604\u070f\u17b4\u17b5\u200c-\u200f\u2028-\u202f\u2060-\u206f\ufeff\ufff0-\uffff]/g;"function"!=typeof Date.prototype.toJSON&&(Date.prototype.toJSON=function(){return isFinite(this.valueOf())?this.getUTCFullYear()+"-"+f(this.getUTCMonth()+1)+"-"+f(this.getUTCDate())+"T"+f(this.getUTCHours())+":"+f(this.getUTCMinutes())+":"+f(this.getUTCSeconds())+"Z":null},Boolean.prototype.toJSON=this_value,Number.prototype.toJSON=this_value,String.prototype.toJSON=this_value);var gap,indent,meta,rep;"function"!=typeof JSON.stringify&&(meta={"\b":"\\b","\t":"\\t","\n":"\\n","\f":"\\f","\r":"\\r",'"':'\\"',"\\":"\\\\"},JSON.stringify=function(a,b,c){var d;if(gap="",indent="","number"==typeof c)for(d=0;d<c;d+=1)indent+=" ";else"string"==typeof c&&(indent=c);if(rep=b,b&&"function"!=typeof b&&("object"!=typeof b||"number"!=typeof b.length))throw new Error("JSON.stringify");return str("",{"":a})}),"function"!=typeof JSON.parse&&(JSON.parse=function(text,reviver){function walk(a,b){var c,d,e=a[b];if(e&&"object"==typeof e)for(c in e)Object.prototype.hasOwnProperty.call(e,c)&&(d=walk(e,c),void 0!==d?e[c]=d:delete e[c]);return reviver.call(a,b,e)}var j;if(text=String(text),rx_dangerous.lastIndex=0,rx_dangerous.test(text)&&(text=text.replace(rx_dangerous,function(a){return"\\u"+("0000"+a.charCodeAt(0).toString(16)).slice(-4)})),rx_one.test(text.replace(rx_two,"@").replace(rx_three,"]").replace(rx_four,"")))return j=eval("("+text+")"),"function"==typeof reviver?walk({"":j},""):j;throw new SyntaxError("JSON.parse")})}();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Generic utils
////////////////////////////////////////////////////////////////////////////////////////////////////

function last(array) {
  return array[array.length - 1];
}

/** Creates a random string in GUID format. */
function createGuid() {
  return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function (c) {
    var r = (Math.random() * 16) | 0;
    var v = c === 'x' ? r : (r & 0x3) | 0x8;
    return v.toString(16);
  });
}

/** Converts the given list-like value to an array. */
function toArray(list) {
  // Some AE collections are 1-based
  var offset = list instanceof LayerCollection || list instanceof ItemCollection ? 1 : 0;

  var result = [];
  for (var i = 0; i < list.length; i++) {
    result.push(list[i + offset]);
  }
  return result;
}

/**
 * Replaces all `{}` placeholders in the format string with the given values.
 *
 * @param format - The format string.
 * @param {...*} args - The values to insert into the format string.
 */
function format(format) {
  var args = toArray(arguments).slice(1);
  var index = 0;
  return format.replace(/\{(\d*)\}/g, function (_, indexString) {
    return String(args[indexString !== '' ? parseInt(indexString) : index++]);
  });
}

/** Indicates whether we're on a Windows machine. If `false`, we're on macOS. */
var osIsWindows = (system.osName || $.os).match(/windows/i);

/**
 * Joins the given path elements with the OS-appropriate path separator.
 *
 * @param {...string} elements - The path elements.
 */
function joinPath() {
  var separator = osIsWindows ? '\\' : '/';
  return toArray(arguments).join(separator);
}

/** Returns a `File` object in the temp directory with a unique file name based on the given one. */
function getTempFile(fileName) {
  var fileNameElements = fileName.split('.');
  fileNameElements.splice(fileNameElements.length > 1 ? -1 : 1, 0, createGuid());
  var uniqueFileName = fileNameElements.join('.');

  return new File(joinPath(Folder.temp.fsName, uniqueFileName));
}

/** Indicates whether this AE script is allowed to write files. */
function canWriteFiles() {
  try {
    var file = new File();
    file.open('w');
    file.writeln('');
    file.close();
    file.remove();
    return true;
  } catch (e) {
    return false;
  }
}

/** Returns the string contents of the given file. */
function readTextFile(fileOrPath) {
  var filePath = fileOrPath.fsName || fileOrPath;
  var file = new File(filePath);
  function check() {
    if (file.error) throw new Error(format('Error reading file "{}": {}', filePath, file.error));
  }

  try {
    file.open('r');
    check();
    file.encoding = 'UTF-8';
    check();
    var result = file.read();
    check();
    file.close();
    check();
    return result;
  } catch (error) {
    try {
      file.close();
    } catch (e) {}
    throw error;
  }
}

/** Creates of overwrites a file with the given string contents. */
function writeTextFile(fileOrPath, text) {
  var filePath = fileOrPath.fsName || fileOrPath;
  var file = new File(filePath);
  function check() {
    if (file.error) throw new Error(format('Error writing file "{}": {}', filePath, file.error));
  }

  try {
    file.open('w');
    check();
    file.encoding = 'UTF-8';
    check();
    file.write(text);
    check();
    file.close();
    check();
  } catch (error) {
    try {
      file.close();
    } catch (e) {}
    throw error;
  }
}

/** Escapes a CLI argument using OS-specific syntax. */
function cliEscape(arg) {
  if (osIsWindows) {
    var trivial = arg.length > 0 && !/[ \t"]/.test(arg);
    return trivial ? arg : format('"{}"', arg.replace(/([\\"])/g, '\\$1'));
  } else {
    var trivial = arg.length > 0 && !/[ \t\\$"'`]/i.test(arg);
    return trivial ? arg : format('"{}"', arg.replace(/([\\$"`])/g, '\\$1'));
  }
}

/**
 * Executes the given program with the given arguments.
 *
 * @param command - The name/path of the executable to run.
 * @param {...*} args - The CLI arguments.
 */
function exec() {
  var commandLine = toArray(arguments).map(cliEscape).join(' ');

  // On macOS, we need to run the command through zsh to get PATH resolution.
  if (!osIsWindows) commandLine = format('zsh -i -c {}', cliEscape(commandLine));

  return system.callSystem(commandLine);
}

/**
 * Shows a dialog with the given message strings or `Error` objects.
 *
 * @param {...*} args - The message strings or `Error` objects to display.
 */
function showErrorDialog() {
  var message = toArray(arguments)
    .map(function (arg) {
      return arg instanceof Error ? arg.message : arg;
    })
    .join('\n\n');
  Window.alert(message, scriptName, true /* show error icon */);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// After Effects-specific utils
////////////////////////////////////////////////////////////////////////////////////////////////////

/** Converts a frame number within a given comp to an offset in seconds. */
function frameToTime(frameNumber, compItem) {
  return frameNumber * compItem.frameDuration;
}

/** Converts an offset in seconds within a given comp to a frame number. */
function timeToFrame(time, compItem) {
  return time * compItem.frameRate;
}

/**
 * Converts a frame number to a timecode string like '00:08'.
 * The frame number is expected to be in the range [0, frame rate[.
 */
function frameToTimecode(frameNumber) {
  return '00:' + String(frameNumber).padStart(2, '0');
}

// To prevent rounding errors
var epsilon = 0.001;

/** Indicates whether the given comp has visible content at the given frame number. */
function isFrameVisible(compItem, frameNumber) {
  if (!compItem) return false;

  var time = frameToTime(frameNumber + epsilon, compItem);
  return toArray(compItem.layers).some(function (layer) {
    return layer.hasVideo && layer.activeAtTime(time);
  });
}

/** Returns the path of a project item within the current project. */
function getItemPath(item) {
  if (item === app.project.rootFolder) return '/';
  if (item.parentFolder === app.project.rootFolder) return format('/ {}', item.name);
  return format('{} / {}', getItemPath(item.parentFolder), item.name);
}

/** Returns all project items representing audio files. */
function getAudioFileProjectItems() {
  return toArray(app.project.items).filter(function (item) {
    var isAudioFootage = item instanceof FootageItem && item.hasAudio && !item.hasVideo;
    return isAudioFootage;
  });
}

/**
 * In the given item control, selects the item with the specified text.
 * If no such item exists, selects the first item, if present.
 */
function selectByTextOrFirst(itemControl, text) {
  var targetItem = toArray(itemControl.items).find(function (item) {
    return item.text === text;
  });
  if (!targetItem && itemControl.items.length) {
    targetItem = itemControl.items[0];
  }
  if (targetItem) {
    itemControl.selection = targetItem;
  }
}

/**
 * Takes a callback returning a tree of controls created using factory functions and converts it
 * into an ExtendScript resource string.
 * This is less painful then writing the resource string manually.
 */
function createResourceString(callback) {
  // prettier-ignore
  var controlTypes = [
    'dialog', 'palette', 'Panel', 'Group', 'TabbedPanel', 'Tab', 'Button', 'IconButton', 'Image',
    'StaticText', 'EditText', 'Checkbox', 'RadioButton', 'Progressbar', 'Slider', 'Scrollbar',
    'ListBox', 'DropDownList', 'TreeView', 'ListItem', 'FlashPlayer'
  ];
  var factories = Object.fromEntries(
    controlTypes.map(function (type) {
      var key = type[0].toLowerCase() + type.slice(1);
      return [
        key,
        function (options) {
          return Object.assign({ __type__: type }, options);
        }
      ];
    })
  );

  var tree = callback(factories);
  var jsonTree = JSON.stringify(tree, null, 2);
  return jsonTree.replace(/(\{\s*)"__type__":\s*"(\w+)",?\s*/g, '$2 $1');
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Rhubarb-specific utils
////////////////////////////////////////////////////////////////////////////////////////////////////

var settingsFilePath = Folder.userData.fullName + '/rhubarb-ae-settings.json';

/**
 * Returns the plugin settings previously stored using {@link writeSettingsFile}.
 * Returns an empty object if no stored settings exist.
 */
function readSettingsFile() {
  try {
    return JSON.parse(readTextFile(settingsFilePath));
  } catch (e) {
    return {};
  }
}

/** Stores an object containing plugin settings for later retrieval. */
function writeSettingsFile(settings) {
  try {
    writeTextFile(settingsFilePath, JSON.stringify(settings, null, 2));
  } catch (e) {
    throw new Error(format('Error persisting settings. {}', e.message));
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Script-specific code
////////////////////////////////////////////////////////////////////////////////////////////////////

var mouthShapeNames = 'ABCDEFGHX'.split('');
var basicMouthShapeCount = 6;
var mouthShapeCount = mouthShapeNames.length;
var basicMouthShapeNames = mouthShapeNames.slice(0, basicMouthShapeCount);
var extendedMouthShapeNames = mouthShapeNames.slice(basicMouthShapeCount);

function getMouthCompHelpTip() {
  var table = mouthShapeNames
    .map(function (mouthShapeName, i) {
      var isOptional = i >= basicMouthShapeCount;
      return format(
        '{}\t{}',
        frameToTimecode(i),
        mouthShapeName + (isOptional ? ' (optional)' : '')
      );
    })
    .join('\n');
  return (
    'A composition containing the mouth shapes, one drawing per frame. ' +
    'They must be arranged as follows:\n' +
    table
  );
}

function createDialogWindow() {
  var resourceString = createResourceString(function (f) {
    return f.dialog({
      text: scriptName,
      settings: f.group({
        orientation: 'column',
        alignChildren: ['left', 'top'],
        audioFile: f.group({
          label: f.staticText({
            text: 'Audio file:',
            // If I don't explicitly activate a control, After Effects has trouble
            // with keyboard focus, so I can't type in the text edit field below.
            active: true
          }),
          value: f.dropDownList({
            helpTip:
              'An audio file containing recorded dialog.\n' +
              'This field shows all audio files that exist in your After Effects project.'
          })
        }),
        recognizer: f.group({
          label: f.staticText({ text: 'Recognizer:' }),
          value: f.dropDownList({
            helpTip: 'The dialog recognizer.'
          })
        }),
        dialogText: f.group({
          label: f.staticText({ text: 'Dialog text (optional):' }),
          value: f.editText({
            properties: { multiline: true },
            characters: 60,
            minimumSize: [0, 100],
            helpTip:
              'For better animation results, you can specify the text of the recording here. ' +
              'This field is optional.'
          })
        }),
        mouthComp: f.group({
          label: f.staticText({ text: 'Mouth composition:' }),
          value: f.dropDownList({ helpTip: getMouthCompHelpTip() })
        }),
        extendedMouthShapes: f.group(
          Object.assign(
            { label: f.staticText({ text: 'Extended mouth shapes:' }) },
            Object.fromEntries(
              extendedMouthShapeNames.map(function (shapeName) {
                return [
                  shapeName.toLowerCase(),
                  f.checkbox({
                    text: shapeName,
                    helpTip: format('Controls whether to use the optional {} shape.', shapeName)
                  })
                ];
              })
            )
          )
        ),
        targetFolder: f.group({
          label: f.staticText({ text: 'Target folder:' }),
          value: f.dropDownList({
            helpTip:
              'The project folder in which to create the animation composition. ' +
              'The composition will be named like the audio file.'
          })
        }),
        frameRate: f.group({
          label: f.staticText({ text: 'Frame rate:' }),
          value: f.editText({
            characters: 8,
            helpTip: 'The frame rate for the animation.'
          }),
          auto: f.checkbox({
            text: 'From mouth composition',
            helpTip:
              'If checked, the animation will use the same frame rate as the mouth composition.'
          })
        })
      }),
      separator: f.group({ preferredSize: ['', 3] }),
      warningLabel: osIsWindows
        ? null
        : f.staticText({
            text:
              'Note: Generating the animation may take a while. ' +
              'During this time, After Effects will not be responsive.'
          }),
      buttons: f.group({
        alignment: 'right',
        animate: f.button({
          properties: { name: 'ok' },
          text: 'Animate'
        }),
        cancel: f.button({
          properties: { name: 'cancel' },
          text: 'Cancel'
        })
      })
    });
  });

  // Create window
  var window = new Window(resourceString);

  // Get controls
  var controls = {
    audioFile: window.settings.audioFile.value,
    dialogText: window.settings.dialogText.value,
    recognizer: window.settings.recognizer.value,
    mouthComp: window.settings.mouthComp.value,
    targetFolder: window.settings.targetFolder.value,
    frameRate: window.settings.frameRate.value,
    autoFrameRate: window.settings.frameRate.auto,
    animateButton: window.buttons.animate,
    cancelButton: window.buttons.cancel
  };
  extendedMouthShapeNames.forEach(function (shapeName) {
    controls['mouthShape' + shapeName] =
      window.settings.extendedMouthShapes[shapeName.toLowerCase()];
  });

  // Add drop down options

  // ...audio file
  getAudioFileProjectItems().forEach(function (projectItem) {
    var listItem = controls.audioFile.add('item', getItemPath(projectItem));
    listItem.projectItem = projectItem;
  });

  // ...recognizer
  var recognizerOptions = [
    { text: 'PocketSphinx (use for English recordings)', value: 'pocketSphinx' },
    { text: 'Phonetic (use for non-English recordings)', value: 'phonetic' }
  ];
  recognizerOptions.forEach(function (option) {
    var listItem = controls.recognizer.add('item', option.text);
    listItem.value = option.value;
  });

  // ...mouth composition
  var comps = toArray(app.project.items).filter(function (item) {
    return item instanceof CompItem;
  });
  comps.forEach(function (comp) {
    var listItem = controls.mouthComp.add('item', getItemPath(comp));
    listItem.projectItem = comp;
  });

  // ...target folder
  var projectFolders = toArray(app.project.items).filter(function (item) {
    return item instanceof FolderItem;
  });
  projectFolders.unshift(app.project.rootFolder);
  projectFolders.forEach(function (projectFolder) {
    var listItem = controls.targetFolder.add('item', getItemPath(projectFolder));
    listItem.projectItem = projectFolder;
  });

  // Load persisted settings
  var settings = readSettingsFile();
  selectByTextOrFirst(controls.audioFile, settings.audioFile);
  controls.dialogText.text = settings.dialogText || '';
  selectByTextOrFirst(controls.recognizer, settings.recognizer);
  selectByTextOrFirst(controls.mouthComp, settings.mouthComp);
  extendedMouthShapeNames.forEach(function (shapeName) {
    controls['mouthShape' + shapeName].value = (settings.extendedMouthShapes || {})[
      shapeName.toLowerCase()
    ];
  });
  selectByTextOrFirst(controls.targetFolder, settings.targetFolder);
  controls.frameRate.text = settings.frameRate || '';
  controls.autoFrameRate.value = settings.autoFrameRate;

  // Align controls
  window.onShow = function () {
    // Give uniform width to all labels
    var groups = toArray(window.settings.children).filter(function (control) {
      return control instanceof Group;
    });
    var labelWidths = groups.map(function (group) {
      return group.children[0].size.width;
    });
    var maxLabelWidth = Math.max.apply(Math, labelWidths);
    groups.forEach(function (group) {
      group.children[0].size.width = maxLabelWidth;
    });

    // Give uniform width to inputs
    var valueWidths = groups.map(function (group) {
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

  var updating = false;

  function update() {
    if (updating) return;

    updating = true;
    try {
      // Handle auto frame rate
      var autoFrameRate = controls.autoFrameRate.value;
      controls.frameRate.enabled = !autoFrameRate;
      if (autoFrameRate) {
        // Take frame rate from mouth comp
        var mouthComp = (controls.mouthComp.selection || {}).projectItem;
        controls.frameRate.text = mouthComp ? mouthComp.frameRate : '';
      } else {
        // Sanitize frame rate
        var sanitizedFrameRate = controls.frameRate.text.match(/\d*\.?\d*/)[0];
        if (sanitizedFrameRate !== controls.frameRate.text) {
          controls.frameRate.text = sanitizedFrameRate;
        }
      }

      // Store settings
      var settings = {
        audioFile: (controls.audioFile.selection || {}).text,
        recognizer: (controls.recognizer.selection || {}).text,
        dialogText: controls.dialogText.text,
        mouthComp: (controls.mouthComp.selection || {}).text,
        extendedMouthShapes: {},
        targetFolder: (controls.targetFolder.selection || {}).text,
        frameRate: Number(controls.frameRate.text),
        autoFrameRate: controls.autoFrameRate.value
      };
      extendedMouthShapeNames.forEach(function (shapeName) {
        settings.extendedMouthShapes[shapeName.toLowerCase()] =
          controls['mouthShape' + shapeName].value;
      });
      writeSettingsFile(settings);
    } finally {
      updating = false;
    }
  }

  // Validate user input. Possible return values:
  // * Non-empty string: Validation failed. Show error message.
  // * Empty string: Validation failed. Don't show error message.
  // * Undefined: Validation succeeded.
  function validate() {
    // Check input values
    if (!controls.audioFile.selection) return 'Please select an audio file.';
    if (!controls.mouthComp.selection) return 'Please select a mouth composition.';
    if (!controls.targetFolder.selection) return 'Please select a target folder.';
    if (Number(controls.frameRate.text) < 12) {
      return 'Please enter a frame rate of at least 12 fps.';
    }

    // Check mouth shape visibility
    var comp = controls.mouthComp.selection.projectItem;
    for (var i = 0; i < mouthShapeCount; i++) {
      var shapeName = mouthShapeNames[i];
      var required = i < basicMouthShapeCount || controls['mouthShape' + shapeName].value;
      if (required && !isFrameVisible(comp, i)) {
        return format(
          'The mouth comp does not seem to contain an image for shape {} at frame {}.',
          shapeName,
          frameToTimecode(i)
        );
      }
    }

    if (!comp.preserveNestedFrameRate) {
      var fix = Window.confirm(
        'The setting "Preserve frame rate when nested or in render queue" is not active for the ' +
          'mouth composition. This can result in incorrect animation.\n\n' +
          'Activate this setting now?',
        false,
        'Fix composition setting?'
      );
      if (fix) {
        app.beginUndoGroup(format('{}: Mouth composition setting', scriptName));
        comp.preserveNestedFrameRate = true;
        app.endUndoGroup();
      } else {
        return '';
      }
    }
  }

  function generateMouthCues(audioFileFootage, recognizer, dialogText, extendedMouthShapeNames) {
    var dialogFile = getTempFile('dialog.txt');
    var logFile = getTempFile('log.txt');
    var jsonFile = getTempFile('output.json');
    try {
      // Create text file containing dialog
      writeTextFile(dialogFile, dialogText);

      // Run Rhubarb
      // prettier-ignore
      var commandParts = [
            'rhubarb',
            '--dialogFile',       dialogFile.fsName,
            '--recognizer',       recognizer,
            '--extendedShapes',   extendedMouthShapeNames.join(''),
            // TODO: Pass the frame rate once Rhubarb does something sensible with it
            '--exportFormat',     'json',
            '--logFile',          logFile.fsName,
            '--logLevel',         'fatal',
            '--output',           jsonFile.fsName,
            audioFileFootage.file.fsName
          ];

      if (osIsWindows) {
        // Run the command via cmd. This opens a new window, letting the user see the progress and
        // abort if desired.
        // cmd treats quotes weirdly; see https://stackoverflow.com/a/62198274
        // Also, cmd treats carets specially, so escape them.
        var commandLine = commandParts.map(cliEscape).join(' ');
        system.callSystem(format('cmd /S /C "{}"', commandLine.replace(/\^/g, '^^')));
      } else {
        // There is no simple way to run a command in a visible window on macOS.
        // The user will have to wait for the command to finish.
        exec.apply(undefined, commandParts);
      }

      // Check log for fatal errors
      if (logFile.exists) {
        var fatalLog = readTextFile(logFile).trim();
        if (fatalLog) {
          // Try to extract only the actual error message
          var match = fatalLog.match(/\[Fatal\] ([\s\S]*)/);
          var message = match ? match[1] : fatalLog;
          throw new Error(format('Error running Rhubarb Lip Sync.\n{}', message));
        }
      }

      try {
        return JSON.parse(readTextFile(jsonFile)).mouthCues;
      } catch (e) {
        throw new Error('No animation result. Animation was probably canceled.');
      }
    } finally {
      dialogFile.remove();
      logFile.remove();
      jsonFile.remove();
    }
  }

  function animateMouthCues(
    mouthCues,
    audioFileFootage,
    mouthComp,
    targetProjectFolder,
    frameRate
  ) {
    // Find a non-conflicting comp name
    // ... strip extension, if present
    var baseName = audioFileFootage.name.match(/^(.*?)(\..*)?$/i)[1];
    var compName = baseName;
    // ... add numeric suffix, if needed
    var existingItems = toArray(targetProjectFolder.items);
    var counter = 1;
    while (
      existingItems.some(function (item) {
        return item.name === compName;
      })
    ) {
      counter++;
      compName = format('{} {}', baseName, counter);
    }

    // Create new comp
    var comp = targetProjectFolder.items.addComp(
      compName,
      mouthComp.width,
      mouthComp.height,
      mouthComp.pixelAspect,
      audioFileFootage.duration,
      frameRate
    );

    // Show new comp
    comp.openInViewer();

    // Add audio layer
    comp.layers.add(audioFileFootage);

    // Add mouth layer
    var mouthLayer = comp.layers.add(mouthComp);
    mouthLayer.timeRemapEnabled = true;
    mouthLayer.outPoint = comp.duration;

    // Animate mouth layer
    var timeRemap = mouthLayer['Time Remap'];
    // Enabling time remapping automatically adds two keys. Remove the second.
    timeRemap.removeKey(2);
    mouthCues.forEach(function (mouthCue) {
      // Round down keyframe time. In animation, earlier is better than later.
      // Set keyframe time to *just before* the exact frame to prevent rounding errors
      var frame = Math.floor(timeToFrame(mouthCue.start, comp));
      var time = frame !== 0 ? frameToTime(frame - epsilon, comp) : 0;
      // Set remapped time to *just after* the exact frame to prevent rounding errors
      var mouthCompFrame = mouthShapeNames.indexOf(mouthCue.value);
      var remappedTime = frameToTime(mouthCompFrame + epsilon, mouthComp);
      timeRemap.setValueAtTime(time, remappedTime);
    });
    for (var i = 1; i <= timeRemap.numKeys; i++) {
      timeRemap.setInterpolationTypeAtKey(i, KeyframeInterpolationType.HOLD);
    }
  }

  function animate(
    audioFileFootage,
    recognizer,
    dialogText,
    mouthComp,
    extendedMouthShapeNames,
    targetProjectFolder,
    frameRate
  ) {
    try {
      var mouthCues = generateMouthCues(
        audioFileFootage,
        recognizer,
        dialogText,
        extendedMouthShapeNames
      );

      app.beginUndoGroup(format('{}: Animation', scriptName));
      animateMouthCues(mouthCues, audioFileFootage, mouthComp, targetProjectFolder, frameRate);
      app.endUndoGroup();
    } catch (e) {
      showErrorDialog('Error creating animation.', e);
      return;
    }
  }

  // Handle changes
  update();
  controls.audioFile.onChange = update;
  controls.recognizer.onChange = update;
  controls.dialogText.onChanging = update;
  controls.mouthComp.onChange = update;
  extendedMouthShapeNames.forEach(function (shapeName) {
    controls['mouthShape' + shapeName].onClick = update;
  });
  controls.targetFolder.onChange = update;
  controls.frameRate.onChanging = update;
  controls.autoFrameRate.onClick = update;

  // Handle animation
  controls.animateButton.onClick = function () {
    var validationError = validate();
    if (typeof validationError === 'string') {
      if (validationError !== '') {
        showErrorDialog(validationError);
      }
    } else {
      window.close();
      animate(
        controls.audioFile.selection.projectItem,
        controls.recognizer.selection.value,
        controls.dialogText.text || '',
        controls.mouthComp.selection.projectItem,
        extendedMouthShapeNames.filter(function (shapeName) {
          return controls['mouthShape' + shapeName].value;
        }),
        controls.targetFolder.selection.projectItem,
        Number(controls.frameRate.text)
      );
    }
  };

  // Handle cancellation
  controls.cancelButton.onClick = function () {
    window.close();
  };

  return window;
}

function checkPreconditions() {
  // Check for file system access
  if (!canWriteFiles()) {
    var section = app.version >= '22' ? 'Scripting & Expressions' : 'General';
    throw new Error(
      format(
        'This script requires file system access.\n\n' +
          'Please enable Preferences > {} > Allow Scripts to Write Files and Access Network.',
        section
      )
    );
  }

  // Check for correct Rhubarb version
  var versionString = exec('rhubarb', '--version') || '';
  var match = versionString.match(
    /Rhubarb Lip Sync version ((\d+)\.(\d+).(\d+)(-[0-9A-Za-z-.]+)?)/
  );
  if (!match) {
    throw new Error(
      'Cannot find Rhubarb Lip Sync.\n' +
        'Make sure your PATH environment variable contains the Rhubarb application directory.'
    );
  }
  var versionString = match[1];
  var major = Number(match[2]);
  var minor = Number(match[3]);
  var requiredMajor = 1;
  var minRequiredMinor = 9;
  if (major !== requiredMajor || minor < minRequiredMinor) {
    throw new Error(
      format(
        'This script requires Rhubarb Lip Sync {}.{}.0 or a later {0}.x version. ' +
          'Your installed version is {}, which is not compatible.',
        requiredMajor,
        minRequiredMinor,
        versionString
      )
    );
  }
}

function main() {
  try {
    loadPolyfills();
    checkPreconditions();
    createDialogWindow().show();
  } catch (error) {
    showErrorDialog(error);
  }
}

main();
