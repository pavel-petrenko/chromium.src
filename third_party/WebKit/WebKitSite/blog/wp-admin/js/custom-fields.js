jQuery(document).ready(function(d){var c,b,e,a;c=function(){var g=d("#newmeta [name=_ajax_nonce]").val(),f=d("#post_ID").val();if(!g||!f){return false}return[g,f]};b=function(g){var f=c();if(!f){return false}g.data=g.data.replace(/_ajax_nonce=[a-f0-9]+/,"_ajax_nonce="+f[0])+"&post_id="+f[1];return g};e=function(j,i){var f=d("postid",j).text(),g;if(!f){return}d("#post_ID").attr("name","post_ID").val(f);g=d("#hiddenaction");if("post"==g.val()){g.val("postajaxpost")}};a=function(g){var f=c();if(!f){return false}g.data._ajax_nonce=f[0];g.data.post_id=f[1];return g};d("#the-list").wpList({addBefore:b,addAfter:e,delBefore:a}).find(".updatemeta, .deletemeta").attr("type","button")});