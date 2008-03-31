(function(){var Event=tinymce.dom.Event;tinymce.create('tinymce.plugins.PastePlugin',{init:function(ed,url){var t=this;t.editor=ed;ed.addCommand('mcePasteText',function(ui,v){if(ui){if((ed.getParam('paste_use_dialog',true))||(!tinymce.isIE)){ed.windowManager.open({file:url+'/pastetext.htm',width:450,height:400,inline:1},{plugin_url:url});}else t._insertText(clipboardData.getData("Text"),true);}else t._insertText(v.html,v.linebreaks);});ed.addCommand('mcePasteWord',function(ui,v){if(ui){if((ed.getParam('paste_use_dialog',true))||(!tinymce.isIE)){ed.windowManager.open({file:url+'/pasteword.htm',width:450,height:400,inline:1},{plugin_url:url});}else t._insertText(t._clipboardHTML());}else t._insertWordContent(v);});ed.addCommand('mceSelectAll',function(){ed.execCommand('selectall');});ed.addButton('pastetext',{title:'paste.paste_text_desc',cmd:'mcePasteText',ui:true});ed.addButton('pasteword',{title:'paste.paste_word_desc',cmd:'mcePasteWord',ui:true});ed.addButton('selectall',{title:'paste.selectall_desc',cmd:'mceSelectAll'});if(ed.getParam("paste_auto_cleanup_on_paste",false)){ed.onPaste.add(function(ed,e){return t._handlePasteEvent(e)});}if(!tinymce.isIE&&ed.getParam("paste_auto_cleanup_on_paste",false)){ed.onKeyDown.add(function(ed,e){if(e.ctrlKey&&e.keyCode==86){window.setTimeout(function(){ed.execCommand("mcePasteText",true);},1);Event.cancel(e);}});}},getInfo:function(){return{longname:'Paste text/word',author:'Moxiecode Systems AB',authorurl:'http://tinymce.moxiecode.com',infourl:'http://wiki.moxiecode.com/index.php/TinyMCE:Plugins/paste',version:tinymce.majorVersion+"."+tinymce.minorVersion};},_handlePasteEvent:function(e){var html=this._clipboardHTML(),ed=this.editor,sel=ed.selection,r;if(ed&&(r=sel.getRng())&&r.text.length>0)ed.execCommand('delete');if(html&&html.length>0)ed.execCommand('mcePasteWord',false,html);return Event.cancel(e);},_insertText:function(content,bLinebreaks){if(content&&content.length>0){if(bLinebreaks){if(this.editor.getParam("paste_create_paragraphs",true)){var rl=this.editor.getParam("paste_replace_list",'\u2122,<sup>TM</sup>,\u2026,...,\u201c|\u201d,",\u2019,\',\u2013|\u2014|\u2015|\u2212,-').split(',');for(var i=0;i<rl.length;i+=2)content=content.replace(new RegExp(rl[i],'gi'),rl[i+1]);content=content.replace(/\r\n\r\n/g,'</p><p>');content=content.replace(/\r\r/g,'</p><p>');content=content.replace(/\n\n/g,'</p><p>');if((pos=content.indexOf('</p><p>'))!=-1){this.editor.execCommand("Delete");var node=this.editor.selection.getNode();var breakElms=[];do{if(node.nodeType==1){if(node.nodeName=="TD"||node.nodeName=="BODY")break;breakElms[breakElms.length]=node;}}while(node=node.parentNode);var before="",after="</p>";before+=content.substring(0,pos);for(var i=0;i<breakElms.length;i++){before+="</"+breakElms[i].nodeName+">";after+="<"+breakElms[(breakElms.length-1)-i].nodeName+">";}before+="<p>";content=before+content.substring(pos+7)+after;}}if(this.editor.getParam("paste_create_linebreaks",true)){content=content.replace(/\r\n/g,'<br />');content=content.replace(/\r/g,'<br />');content=content.replace(/\n/g,'<br />');}}this.editor.execCommand("mceInsertRawHTML",false,content);}},_insertWordContent:function(content){var t=this,ed=t.editor;if(content&&content.length>0){var bull=String.fromCharCode(8226);var middot=String.fromCharCode(183);if(ed.getParam('paste_insert_word_content_callback'))content=ed.execCallback('paste_insert_word_content_callback','before',content);var rl=ed.getParam("paste_replace_list",'\u2122,<sup>TM</sup>,\u2026,...,\u201c|\u201d,",\u2019,\',\u2013|\u2014|\u2015|\u2212,-').split(',');for(var i=0;i<rl.length;i+=2)content=content.replace(new RegExp(rl[i],'gi'),rl[i+1]);if(this.editor.getParam("paste_convert_headers_to_strong",false)){content=content.replace(new RegExp('<p class=MsoHeading.*?>(.*?)<\/p>','gi'),'<p><b>$1</b></p>');}content=content.replace(new RegExp('tab-stops: list [0-9]+.0pt">','gi'),'">'+"--list--");content=content.replace(new RegExp(bull+"(.*?)<BR>","gi"),"<p>"+middot+"$1</p>");content=content.replace(new RegExp('<SPAN style="mso-list: Ignore">','gi'),"<span>"+bull);content=content.replace(/<o:p><\/o:p>/gi,"");content=content.replace(new RegExp('<br style="page-break-before: always;.*>','gi'),'-- page break --');content=content.replace(new RegExp('<(!--)([^>]*)(--)>','g'),"");if(this.editor.getParam("paste_remove_spans",true))content=content.replace(/<\/?span[^>]*>/gi,"");if(this.editor.getParam("paste_remove_styles",true))content=content.replace(new RegExp('<(\\w[^>]*) style="([^"]*)"([^>]*)','gi'),"<$1$3");content=content.replace(/<\/?font[^>]*>/gi,"");switch(this.editor.getParam("paste_strip_class_attributes","all")){case"all":content=content.replace(/<(\w[^>]*) class=([^ |>]*)([^>]*)/gi,"<$1$3");break;case"mso":content=content.replace(new RegExp('<(\\w[^>]*) class="?mso([^ |>]*)([^>]*)','gi'),"<$1$3");break;}content=content.replace(new RegExp('href="?'+this._reEscape(""+document.location)+'','gi'),'href="'+this.editor.documentBaseURI.getURI());content=content.replace(/<(\w[^>]*) lang=([^ |>]*)([^>]*)/gi,"<$1$3");content=content.replace(/<\\?\?xml[^>]*>/gi,"");content=content.replace(/<\/?\w+:[^>]*>/gi,"");content=content.replace(/-- page break --\s*<p>&nbsp;<\/p>/gi,"");content=content.replace(/-- page break --/gi,"");if(!this.editor.getParam('force_p_newlines')){content=content.replace('','','gi');content=content.replace('</p>','<br /><br />','gi');}if(!tinymce.isIE&&!this.editor.getParam('force_p_newlines')){content=content.replace(/<\/?p[^>]*>/gi,"");}content=content.replace(/<\/?div[^>]*>/gi,"");if(this.editor.getParam("paste_convert_middot_lists",true)){var div=ed.dom.create("div",null,content);var className=this.editor.getParam("paste_unindented_list_class","unIndentedList");while(this._convertMiddots(div,"--list--"));while(this._convertMiddots(div,middot,className));while(this._convertMiddots(div,bull));content=div.innerHTML;}if(this.editor.getParam("paste_convert_headers_to_strong",false)){content=content.replace(/<h[1-6]>&nbsp;<\/h[1-6]>/gi,'<p>&nbsp;&nbsp;</p>');content=content.replace(/<h[1-6]>/gi,'<p><b>');content=content.replace(/<\/h[1-6]>/gi,'</b></p>');content=content.replace(/<b>&nbsp;<\/b>/gi,'<b>&nbsp;&nbsp;</b>');content=content.replace(/^(&nbsp;)*/gi,'');}content=content.replace(/--list--/gi,"");if(ed.getParam('paste_insert_word_content_callback'))content=ed.execCallback('paste_insert_word_content_callback','after',content);this.editor.execCommand("mceInsertContent",false,content);if(this.editor.getParam('paste_force_cleanup_wordpaste',true)){var ed=this.editor;window.setTimeout(function(){ed.execCommand("mceCleanup");},1);}}},_reEscape:function(s){var l="?.\\*[](){}+^$:";var o="";for(var i=0;i<s.length;i++){var c=s.charAt(i);if(l.indexOf(c)!=-1)o+='\\'+c;else o+=c;}return o;},_convertMiddots:function(div,search,class_name){var ed=this.editor,mdot=String.fromCharCode(183),bull=String.fromCharCode(8226);var nodes,prevul,i,p,ul,li,np,cp,li;nodes=div.getElementsByTagName("p");for(i=0;i<nodes.length;i++){p=nodes[i];if(p.innerHTML.indexOf(search)==0){ul=ed.dom.create("ul");if(class_name)ul.className=class_name;li=ed.dom.create("li");li.innerHTML=p.innerHTML.replace(new RegExp(''+mdot+'|'+bull+'|--list--|&nbsp;',"gi"),'');ul.appendChild(li);np=p.nextSibling;while(np){if(np.nodeType==3&&new RegExp('^\\s$','m').test(np.nodeValue)){np=np.nextSibling;continue;}if(search==mdot){if(np.nodeType==1&&new RegExp('^o(\\s+|&nbsp;)').test(np.innerHTML)){if(!prevul){prevul=ul;ul=ed.dom.create("ul");prevul.appendChild(ul);}np.innerHTML=np.innerHTML.replace(/^o/,'');}else{if(prevul){ul=prevul;prevul=null;}if(np.nodeType!=1||np.innerHTML.indexOf(search)!=0)break;}}else{if(np.nodeType!=1||np.innerHTML.indexOf(search)!=0)break;}cp=np.nextSibling;li=ed.dom.create("li");li.innerHTML=np.innerHTML.replace(new RegExp(''+mdot+'|'+bull+'|--list--|&nbsp;',"gi"),'');np.parentNode.removeChild(np);ul.appendChild(li);np=cp;}p.parentNode.replaceChild(ul,p);return true;}}return false;},_clipboardHTML:function(){var div=document.getElementById('_TinyMCE_clipboardHTML');if(!div){var div=document.createElement('DIV');div.id='_TinyMCE_clipboardHTML';with(div.style){visibility='hidden';overflow='hidden';position='absolute';width=1;height=1;}document.body.appendChild(div);}div.innerHTML='';var rng=document.body.createTextRange();rng.moveToElementText(div);rng.execCommand('Paste');var html=div.innerHTML;div.innerHTML='';return html;}});tinymce.PluginManager.add('paste',tinymce.plugins.PastePlugin);})();