using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.Filters;
using Microsoft.AspNetCore.Mvc.ApplicationModels;
using Microsoft.AspNetCore.Mvc.ApplicationParts;
using Microsoft.AspNetCore.Mvc.Controllers;
using Microsoft.AspNetCore.Mvc.Routing;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

namespace Server.Web
{
    // Registers exactly RegisteredControllers.All - not ASP.NET's default "every public
    // ControllerBase subclass in the assembly" discovery - so that list stays the single source of
    // truth shared with OpenApiGenerator (a controller reachable over HTTP is exactly a controller
    // documented in /api/openapi.json, and vice versa). Also lets the controllers stay `internal`.
    public sealed class RegisteredControllerFeatureProvider : IApplicationFeatureProvider<ControllerFeature>
    {
        public void PopulateFeature(IEnumerable<ApplicationPart> parts, ControllerFeature feature)
        {
            feature.Controllers.Clear();
            foreach (var type in RegisteredControllers.All)
                feature.Controllers.Add(type.GetTypeInfo());
        }
    }

    // 400 for a query/route value that can't be converted to its parameter type (e.g.
    // ?Enabled=maybe, or an enum given a name it doesn't have) - EmbedIO rejected those up front.
    // Without [ApiController] (deliberately not used: its automatic binding-source inference and
    // problem-details bodies would change far more than this), MVC would instead record the error,
    // bind default(T), and run the action anyway, turning a bad request into a confusing 500 from
    // deep inside native code. A key that's simply ABSENT is not an error here - it still binds
    // the parameter's default, exactly as before.
    public sealed class RejectUnparseableParametersFilter : IActionFilter
    {
        public void OnActionExecuting(ActionExecutingContext context)
        {
            if (context.ModelState.IsValid) return;
            var errors = context.ModelState
                .Where(kv => kv.Value?.Errors.Count > 0)
                .Select(kv => $"{kv.Key}: {string.Join("; ", kv.Value!.Errors.Select(e => e.ErrorMessage))}");
            context.Result = new ContentResult
            {
                StatusCode = 400,
                ContentType = "text/plain; charset=utf-8",
                Content = "invalid parameter(s) - " + string.Join(" | ", errors),
            };
        }

        public void OnActionExecuted(ActionExecutedContext context) { }
    }

    // Prefixes every attribute route with "api/" - the ASP.NET equivalent of EmbedIO's
    // WithWebApi("/api", ...) module base path. Keeps each controller's own route templates (and
    // therefore OpenApiGenerator's documented paths, which have never included /api) unchanged.
    public sealed class ApiPrefixConvention : IApplicationModelConvention
    {
        private readonly AttributeRouteModel _prefix = new(new RouteAttribute("api"));

        public void Apply(ApplicationModel application)
        {
            foreach (var selector in application.Controllers.SelectMany(c => c.Actions).SelectMany(a => a.Selectors))
            {
                if (selector.AttributeRouteModel != null)
                    selector.AttributeRouteModel = AttributeRouteModel.CombineAttributeRouteModel(_prefix, selector.AttributeRouteModel);
            }
        }
    }
}
