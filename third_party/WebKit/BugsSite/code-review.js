(function() {
  function determineAttachmentID() {
    try {
      return /id=(\d+)/.exec(window.location.search)[1]
    } catch (ex) {
      return;
    }
  }

  // Attempt to activate only in the "Formatted Diff" context.
  if (window.top != window)
    return;
  var attachment_id = determineAttachmentID();
  if (!attachment_id)
    return;

  var current_comment = {}
  var next_line_id = 0;

  function idForLine(number) {
    return 'line' + number;
  }

  function nextLineID() {
    return idForLine(next_line_id++);
  }

  function forEachLine(callback) {
    for (var i = 0; i < next_line_id; ++i) {
      callback($('#' + idForLine(i)));
    }
  }

  function idify() {
    this.id = nextLineID();
  }

  function hoverify() {
    $(this).hover(function() {
      $(this).addClass('hot');
    },
    function () {
      $(this).removeClass('hot');
    });
  }

  function findCommentPositionFor(line) {
    var position = line;
    while (position.next() && position.next().hasClass('previous_comment'))
      position = position.next();
    return position;
  }

  function findCommentBlockFor(line) {
    var comment_block = findCommentPositionFor(line).next();
    if (!comment_block.hasClass('comment'))
      return;
    return comment_block;
  }

  function insertCommentFor(line, block) {
    findCommentPositionFor(line).after(block);
  }

  function addCommentFor(line) {
    if (line.attr('data-has-comment'))
      return;
    line.attr('data-has-comment', 'true');
    line.addClass('commentContext');

    var comment_block = $('<div class="comment"><textarea data-comment-for="' + line.attr('id') + '"></textarea><div class="actions"><button class="delete">Delete</button></div></div>');
    insertCommentFor(line, comment_block);
    comment_block.children('textarea').focus();
  }

  function addCommentField() {
    var id = $(this).attr('data-comment-for');
    if (!id)
      id = this.id;
    addCommentFor($('#' + id));
  }

  var files = {}

  function addPreviousComment(line, comment_text) {
    var comment_block = $('<div data-comment-for="' + line.attr('id') + '" class="previous_comment"></div>');
    comment_block.text(comment_text).prepend('<div class="reply">Click to reply</div>');
    comment_block.each(hoverify).click(addCommentField);
    insertCommentFor(line, comment_block);
  }

  function displayPreviousComments(comments) {
    for (var i = 0; i < comments.length; ++i) {
      var file_name = comments[i].file_name;
      var line_number = comments[i].line_number;
      var comment_text = comments[i].comment_text;

      var file = files[file_name];

      var query = '.Line .to';
      if (line_number[0] == '-') {
        // The line_number represent a removal.  We need to adjust the query to
        // look at the "from" lines.
        query = '.Line .from';
        // Trim off the '-' control character.
        line_number = line_number.substr(1);
      }

      $(file).find(query).each(function() {
        if ($(this).text() != line_number)
          return;
        var line = $(this).parent();
        addPreviousComment(line, comment_text);
      });
    }
  }

  function scanForComments(text) {
    var comments = []
    var lines = text.split('\n');
    for (var i = 0; i < lines.length; ++i) {
      var parts = lines[i].match(/^([> ]+)([^:]+):(-?\d+)$/);
      if (!parts)
        continue;
      var quote_markers = parts[1];
      var file_name = parts[2];
      var line_number = parts[3];
      if (!file_name in files)
        continue;
      while (i < lines.length && lines[i].length > 0 && lines[i][0] == '>')
        ++i;
      var comment_lines = [];
      while (i < lines.length && (lines[i].length == 0 || lines[i][0] != '>')) {
        comment_lines.push(lines[i]);
        ++i;
      }
      var comment_text = comment_lines.join('\n');
      comments.push({
        'file_name': file_name,
        'line_number': line_number,
        'comment_text': comment_text
      });
    }
    return comments;
  }

  function fetchHistory() {
    $.get('attachment.cgi?id=' + attachment_id + '&action=edit', function(data) {
      var bug_id = /Attachment \d+ Details for Bug (\d+)/.exec(data)[1];
      $.get('show_bug.cgi?id=' + bug_id, function(data) {
        var comments = [];
        $(data).find('.bz_comment').each(function() {
          var author = $(this).find('.email').text();
          var text = $(this).find('.bz_comment_text').text();
          var comment_marker = '(From update of attachment ' + attachment_id + ' .details.)';
          if (text.match(comment_marker))
            $.merge(comments, scanForComments(text));
        });
        displayPreviousComments(comments);
      });
    });
  }

  function crawlDiff() {
    $('.Line').each(idify).each(hoverify).dblclick(addCommentField);
    $('.FileDiff').each(function() {
      var file_name = $(this).children('h1').text();
      files[file_name] = this;
    });
  }

  $(document).ready(function() {
    crawlDiff();
    fetchHistory();
    $(document.body).prepend('<div id="toolbar"><div class="actions"><button id="post_comments">Prepare comments</button></div><div class="help">Double-click a line to add a comment.</div></div>');
    $(document.body).prepend('<div id="comment_form" class="inactive"><div class="winter"></div><div class="lightbox"><iframe src="attachment.cgi?id=' + attachment_id + '&action=reviewform"></iframe></div></div>');
  });

  $('.comment textarea').live('keydown', function() {
    var line_id = $(this).attr('data-comment-for');
    current_comment[line_id] = $(this).val();
  });

  $('.comment .delete').live('click', function() {
    var line_id = $(this).parentsUntil('.comment').parent().find('textarea').attr('data-comment-for');
    delete current_comment[line_id];
    var line = $('#' + line_id)
    findCommentBlockFor(line).remove();
    line.removeAttr('data-has-comment');
    trimCommentContextToBefore(line);
  });

  function contextLinesFor(line) {
    var context = [];
    while (line.hasClass('commentContext')) {
      $.merge(context, line);
      line = line.prev();
    }
    return $(context.reverse());
  }

  function trimCommentContextToBefore(line) {
    while (line.hasClass('commentContext') && line.attr('data-has-comment') != 'true') {
      line.removeClass('commentContext');
      line = line.prev();
    }
  }

  var in_drag_select = false;

  function stopDragSelect() {
    $('.selected').removeClass('selected');
    in_drag_select = false;
  }

  $('.lineNumber').live('click', function() {
    var line = $(this).parent();
    if (line.hasClass('commentContext'))
      trimCommentContextToBefore(line.prev());
  }).live('mousedown', function() {
    in_drag_select = true;
    $(this).parent().addClass('selected');
    event.preventDefault();
  });
  
  $('.Line').live('mouseenter', function() {
    if (!in_drag_select)
      return;

    var before = $(this).prevUntil('.selected')
    if (before.prev().hasClass('selected'))
      before.addClass('selected');

    var after = $(this).nextUntil('.selected')
    if (after.next().hasClass('selected'))
      after.addClass('selected');

    $(this).addClass('selected');
  }).live('mouseup', function() {
    if (!in_drag_select)
      return;
    var selected = $('.selected');
    var should_add_comment = !selected.last().next().hasClass('commentContext');
    selected.addClass('commentContext');
    if (should_add_comment)
      addCommentFor(selected.last());
  });

  $('.DiffSection').live('mouseleave', stopDragSelect).live('mouseup', stopDragSelect);

  function contextSnippetFor(line) {
    var snippets = []
    contextLinesFor(line).each(function() {
      var action = ' ';
      if ($(this).hasClass('add'))
        action = '+';
      else if ($(this).hasClass('remove'))
        action = '-';
      var text = $(this).children('.text').text();
      snippets.push('> ' + action + text);
    });
    return snippets.join('\n');
  }

  function fileNameFor(line) {
    return line.parentsUntil('.FileDiff').parent().find('h1').text();
  }

  function snippetFor(line) {
    var file_name = fileNameFor(line);
    var line_number = line.hasClass('remove') ? '-' + line.children('.from').text() : line.children('.to').text();
    return '> ' + file_name + ':' + line_number + '\n' + contextSnippetFor(line);
  }

  $('#comment_form .winter').live('click', function() {
    $('#comment_form').addClass('inactive');
  });

  $('#post_comments').live('click', function() {
    var comments_in_context = []
    forEachLine(function(line) {
      if (line.attr('data-has-comment') != 'true')
        return;
      var snippet = snippetFor(line);
      var comment = findCommentBlockFor(line).children('textarea').val();
      if (comment == '')
        return;
      comments_in_context.push(snippet + '\n' + comment);
    });
    $('#comment_form').removeClass('inactive');
    var comment = comments_in_context.join('\n\n');
    $('#comment_form').find('iframe').contents().find('#comment').val(comment);
  });
})();
