
_You should have a Neeo-cli server running before attempting to install and run this driver. I have [a github page right here](https://github.com/Webunity/neeo-sdk-webunity/wiki), which not only describes how to set that up, but also how to run this server automatically at boot._

```
cd /opt/<your-neoo-server/
npm install --save git+https://github.com/Webunity/neeo-driver-nvidia-shield
mkdir ./config
cp ./node_modules/neeo-driver-nvidia-shield/config/default.json ./config
```

_If you already had a `config` directory, make sure the contents from the above mentions `default.json` are added to the existing `default.json`._

Assuming you had the Neeo-cli server already running, you should restart your server to load the newly installed driver.

```
systemctl restart neeo-server.service
```
