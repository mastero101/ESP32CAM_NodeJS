const http = require('http');
const formidable = require('formidable');
const fs = require('fs');
const url = require('url');
const cors = require('cors'); // Importa el paquete cors

const server = http.createServer();

server.on('request', (req, res) => {
  // Configuración de los encabezados para permitir solicitudes desde cualquier origen
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  const parsedUrl = url.parse(req.url, true);
  const pathName = parsedUrl.pathname;

  if (pathName === '/upload' && req.method.toLowerCase() === 'post') {
    const form = new formidable.IncomingForm();
    form.parse(req, (err, fields, files) => {
      if (err) {
        res.writeHead(400, { 'Content-Type': 'text/plain' });
        res.end('Bad Request');
        return;
      }

      // Acceder al primer elemento del array de archivos
      const uploadedFile = files.imageFile[0];

      const oldPath = uploadedFile.filepath;
      console.log("Old Path:", oldPath);
      const currentTime = new Date();
      const timezoneOffset = -6; // GMT-06
      const timestamp = new Date(currentTime.getTime() + timezoneOffset * 60 * 60 * 1000).toISOString().replace(/:/g, '-');
      const newPath = `uploads/${timestamp}_${uploadedFile.originalFilename}`;
      console.log("New Path:", newPath);

      fs.copyFile(oldPath, newPath, (copyError) => {
        if (copyError) {
          console.error("Error copying file:", copyError);
          res.writeHead(500, { 'Content-Type': 'text/plain' });
          res.end('Internal Server Error');
          return;
        }
      
        fs.unlink(oldPath, (unlinkError) => {
          if (unlinkError) {
            console.error("Error deleting temporary file:", unlinkError);
            res.writeHead(500, { 'Content-Type': 'text/plain' });
            res.end('Internal Server Error');
            return;
          }
      
          res.writeHead(200, { 'Content-Type': 'text/plain' });
          res.end('File uploaded and moved!');
        });
      });      
    });
  } else if (pathName === '/images' && req.method.toLowerCase() === 'get') {
    fs.readdir('uploads', (err, files) => {
      if (err) {
        res.writeHead(500, { 'Content-Type': 'text/plain' });
        res.end('Internal Server Error');
      } else {
        const imageFiles = files.filter(file => file.match(/\.(jpg|jpeg|png|gif)$/i));
        if (imageFiles.length > 0) {
          res.writeHead(200, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify(imageFiles));
        } else {
          res.writeHead(204, { 'Content-Type': 'text/plain' });
          res.end('No images found');
        }
      }
    });
  } else if (pathName === '/delete' && req.method.toLowerCase() === 'post') {
    let requestBody = '';
    req.on('data', chunk => {
      requestBody += chunk.toString();
    });

    req.on('end', () => {
      try {
        const imageNames = JSON.parse(requestBody);

        // Iterate through the array of image names and delete each image
        imageNames.forEach(imageName => {
          const imagePath = `uploads/${imageName}`;
          fs.unlink(imagePath, err => {
            if (err) {
              console.error("Error deleting image:", err);
            }
          });
        });

        res.writeHead(200, { 'Content-Type': 'text/plain' });
        res.end('Images deleted successfully');
      } catch (error) {
        console.error("Error parsing request body:", error);
        res.writeHead(400, { 'Content-Type': 'text/plain' });
        res.end('Bad Request');
      }
    });
  } else {
    res.writeHead(404, { 'Content-Type': 'text/plain' });
    res.end('Not Found');
  }
});

const PORT = 5001;
server.listen(PORT, () => {
  console.log(`Server listening on port ${PORT}`);
});
