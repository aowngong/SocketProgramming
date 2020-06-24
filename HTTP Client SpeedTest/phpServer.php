<?php<?php
  $targetdir = '/uploads/';   
      // name of the directory where the files should be stored
  $targetfile = $targetdir.$_FILES['file']['name'];

  if (move_uploaded_file($_FILES['file']['tmp_name'], $targetfile)) {
    echo "file uploaded succeeded";
  } else { 
    echo "file upload failed";
  }
?>
