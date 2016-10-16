function open_door() {
    $.ajax({     url: "/openDoor",
            dataType: "json",
               jsonp: false});
    return false;
}

// MAIN ///////////////////////////////////////////////////////////////////////
$(function() {
  load_config();
  check_for_updates('ring');
});
