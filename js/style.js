var clean_uri = location.protocol + "//" + location.host + location.pathname;
window.history.replaceState({}, document.title, clean_uri);
window.onload = function(){
	//Icon
	var x = document.querySelectorAll("a[title=file]");
	for(var i = 0; i < x.length; i++){
		var s = x[i].href;
		if(/(.pdf)$/i.test(s)){
			x[i].parentElement.className = "fa fa-file-pdf-o";
		} else if(/(.txt)$/i.test(s)){
			x[i].parentElement.className = "fa fa-file-text-o";
		} else if(/(.jpg)|(.png)|(.jpeg)|(.gif)$/i.test(s)){
			x[i].parentElement.className = "fa fa-picture-o";
		} else if(/(.mp4)$/i.test(s)){
			x[i].parentElement.className = "fa fa-file-movie-o";
		} else if(/(.zip)|(.rar)$/i.test(s)){
			x[i].parentElement.className = "fa fa-file-zip-o";
		} else if(/(.mp3)$/i.test(s)){
			x[i].parentElement.className = "fa fa-music";
		} else if(/(.doc)|(.docx)$/i.test(s)){
			x[i].parentElement.className = "fa fa-file-word-o";
		} else if(/(.cpp)|(.php)|(.html)|(.css)|(.js)$/i.test(s)){
			x[i].parentElement.className = "fa fa-file-code-o";
		} else if(/(.exe)$/i.test(s)){
			x[i].parentElement.className = "fa fa-window-maximize";
		} else {
			x[i].parentElement.className = "fa fa-file-o";
		}
	}
	x = document.querySelectorAll("a[title=folder]");
	if(x[0].text == '.'){
		x[0].parentElement.className = "0 fa fa-folder-open";
		x[1].parentElement.className = "0 fa fa-folder-open";
	}
	//Sort
	var sort_by_name = function(a, b) {
			return a.className.localeCompare(b.className.toLowerCase());
	}
    
    var list = Array.from(document.querySelectorAll("#listing > div"));
    list.sort(sort_by_name);
	var p = document.getElementById("listing");
    for (var i = 0; i < list.length; i++) {
        p.appendChild(list[i]);
    }
}

function remove(a){
	var b = a.previousSibling.previousSibling;
	if(confirm("Bạn có muốn xoá \"" + b.text + "\"?")){
		$.ajax({
		type : 'DELETE',
		url  : b.href,
		success :	function(data){                       
						if(data === "1"){
							$(b.parentElement).remove();
						} else {
							alert('Có lỗi xảy ra! Không thể xoá file!');
						}
					}
		});
	}
}

function upload(url){
	$("#status").show();
	var sumFile = document.getElementById('multiFiles').files.length;
	document.getElementById('processupload').innerHTML = '<b>Trạng thái:</b><br>&nbsp;&nbsp;&nbsp;&nbsp;Thành công: 0</br>&nbsp;&nbsp;&nbsp;&nbsp;Thất bại: 0</br>&nbsp;&nbsp;&nbsp;&nbsp;Tổng số: ' + sumFile + ' file';
	XuLy(url, 0, sumFile, 0, 0);
}

function XuLy(url, i, sumFile, ok, err){	
	if(i >= sumFile){
		document.getElementById('filenameupload').innerHTML = 'Hoàn tất! Hãy tải lại trang.';
		return 0;
	}
	document.getElementById('filenameupload').innerHTML = 'Đang tải lên: ' + document.getElementById('multiFiles').files[i].name;
	var n = 0;
	var form_data = new FormData();
	form_data.append("files", document.getElementById('multiFiles').files[i]);
	$.ajax({
		url: url,
		dataType: 'text',
		cache: false,
		contentType: false,
		processData: false,
		data: form_data,
		type: 'post',
	}).done(function() {
		ok = ok + 1;
	}).fail(function() {
		err = err + 1;
	}).always(function() {
		document.getElementById('processupload').innerHTML = '<b>Trạng thái:</b><br>&nbsp;&nbsp;&nbsp;&nbsp;Thành công: ' + ok + '</br>&nbsp;&nbsp;&nbsp;&nbsp;Thất bại: ' + err + '</br>&nbsp;&nbsp;&nbsp;&nbsp;Tổng số: ' + sumFile + ' file';
		XuLy(url, i + 1, sumFile, ok, err);
	});
}
