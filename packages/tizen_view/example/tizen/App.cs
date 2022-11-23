using Tizen.Flutter.Embedding;

namespace Runner
{
    public class App : FlutterApplication
    {
        protected override void OnCreate()
        {
            base.OnCreate();

            GeneratedPluginRegistrant.RegisterPlugins(this);


            //registryPlatformView("plugins.flutter.io/tizenview", callback_);
        }

        /*Evas_Object* callback_(Evas_Object* parent)
        {
            return nullptr;
        }*/

        static void Main(string[] args)
        {
            var app = new App();
            app.Run(args);
        }
    }
}
