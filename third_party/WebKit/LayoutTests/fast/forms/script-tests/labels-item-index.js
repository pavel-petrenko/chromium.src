description('Test that the items of a labels list can be accessed individually. ');

var parent = document.createElement('div');

parent.innerHTML = '<div id="div1"></div><div id="div2"><button id="id1"></button><input id="id2"><select id="id3"></select><textarea id="id4"></textarea></div><label id="l1" for="id1"></label><label id="l2" for="id2"></label><label id="l3" for="id3"></label><label id="l4" for="id4"></label><label id="l11" for="id1"></label><label id="l12" for="id2"></label><label id="l13" for="id3"></label><label id="l14" for="id4"></label>';

document.body.appendChild(parent);

labels = document.getElementById("id1").labels;
shouldBe('labels.item(1).id', '"l11"');

labels = document.getElementById("id2").labels;
shouldBe('labels.item(1).id', '"l12"');

labels = document.getElementById("id3").labels;
shouldBe('labels.item(1).id', '"l13"');

labels = document.getElementById("id4").labels;
shouldBe('labels.item(1).id', '"l14"');

var successfullyParsed = true;
