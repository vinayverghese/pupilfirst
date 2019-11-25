open CourseEditor__Types;

let str = ReasonReact.string;

type action =
  | SelectCover(string, bool)
  | SelectThumb(string, bool)
  | BeginUpdate
  | ErrorOccured
  | DoneUpdating;

type state = {
  filenameThumb: option(string),
  filenameCover: option(string),
  invalidThumb: bool,
  invalidCover: bool,
  updating: bool,
  formDirty: bool,
};

let component = ReasonReact.reducerComponent("CourseEditor__ImageHandler");

let updateButtonText = updating => updating ? "Updating..." : "Update Image";

let formId = "course-editor-form-image-form";

let filename = optionalFilename => {
  optionalFilename |> OptionUtils.default("unknown");
};

let handleUpdateCB = (json, state, course, updateCourseCB) => {
  let coverUrl = json |> Json.Decode.(field("cover_url", optional(string)));
  let thumbnailUrl =
    json |> Json.Decode.(field("thumbnail_url", optional(string)));

  let newCourse =
    course
    |> Course.addImages(
         ~coverUrl,
         ~thumbnailUrl,
         ~coverFilename=filename(state.filenameCover),
         ~thumbnailFilename=filename(state.filenameThumb),
       );

  updateCourseCB(newCourse);
};

let handleUpdateImages = (send, state, course, updateCourseCB, event) => {
  event |> ReactEvent.Form.preventDefault;
  send(BeginUpdate);

  let element = ReactDOMRe._getElementById(formId);
  switch (element) {
  | Some(element) =>
    Api.sendFormData(
      "courses/" ++ (course |> Course.id) ++ "/attach_images",
      DomUtils.FormData.create(element),
      json => {
        Notification.success(
          "Done!",
          "Images have been updated successfully.",
        );
        handleUpdateCB(json, state, course, updateCourseCB);
        send(DoneUpdating);
      },
      () => send(ErrorOccured),
    )
  | None => ()
  };
};

let updateButtonDisabled = state =>
  if (state.updating) {
    true;
  } else {
    !state.formDirty || state.invalidThumb || state.invalidCover;
  };

let optionalImageLabelText = (image, selectedFilename) =>
  switch (selectedFilename) {
  | Some(name) =>
    <span>
      {"You have selected " |> str}
      <code className="mr-1"> {name |> str} </code>
      {". Click to replace the current image." |> str}
    </span>
  | None =>
    switch (image) {
    | Some(existingImage) =>
      <span>
        {"Please pick a file to replace " |> str}
        <code> {existingImage |> Course.filename |> str} </code>
      </span>
    | None => "Please choose an image file." |> str
    }
  };

let maxAllowedSize = 2 * 1024 * 1024;

let isInvalidImageFile = image =>
  (
    switch (image##_type) {
    | "image/jpeg"
    | "image/png" => false
    | _ => true
    }
  )
  ||
  image##size > maxAllowedSize;

let updateImage = (send, isCover, event) => {
  let imageFile = ReactEvent.Form.target(event)##files[0];
  isCover
    ? send(SelectCover(imageFile##name, imageFile |> isInvalidImageFile))
    : send(SelectThumb(imageFile##name, imageFile |> isInvalidImageFile));
};

let make = (~course, ~updateCourseCB, ~closeDrawerCB, _children) => {
  ...component,
  initialState: () => {
    filenameThumb: None,
    filenameCover: None,
    invalidThumb: false,
    invalidCover: false,
    updating: false,
    formDirty: false,
  },
  reducer: (action, state) =>
    switch (action) {
    | SelectThumb(name, invalid) =>
      ReasonReact.Update({
        ...state,
        filenameThumb: Some(name),
        invalidThumb: invalid,
        formDirty: true,
      })
    | SelectCover(name, invalid) =>
      ReasonReact.Update({
        ...state,
        filenameCover: Some(name),
        invalidCover: invalid,
        formDirty: true,
      })
    | BeginUpdate => ReasonReact.Update({...state, updating: true})
    | DoneUpdating =>
      ReasonReact.Update({...state, updating: false, formDirty: false})
    | ErrorOccured => ReasonReact.Update({...state, updating: false})
    },
  render: ({state, send}) => {
    let thumbnail = course |> Course.thumbnail;
    let cover = course |> Course.cover;
    <SchoolAdmin__EditorDrawer.Jsx2 closeDrawerCB>
      <form
        id=formId
        className="mx-8 pt-8"
        key="sc-images-editor__form"
        onSubmit={handleUpdateImages(send, state, course, updateCourseCB)}>
        <input
          name="authenticity_token"
          type_="hidden"
          value={AuthenticityToken.fromHead()}
        />
        <h5 className="uppercase text-center border-b border-gray-400 pb-2">
          {"Course Images" |> str}
        </h5>
        <DisablingCover.Jsx2 disabled={state.updating}>
          <div key="course-images-editor__thumbnail" className="mt-4">
            <label
              className="block tracking-wide text-gray-800 text-xs font-semibold"
              htmlFor="sc-images-editor__logo-on-400-bg-input">
              {"Thumbnail" |> str}
            </label>
            <input
              disabled={state.updating}
              className="hidden"
              name="course_thumbnail"
              type_="file"
              accept=".jpg,.jpeg,.png,.gif,image/x-png,image/gif,image/jpeg"
              id="course-images-editor__thumbnail"
              required=false
              multiple=false
              onChange={updateImage(send, false)}
            />
            <label
              className="file-input-label mt-2"
              htmlFor="course-images-editor__thumbnail">
              <i className="fas fa-upload" />
              <span className="ml-2 truncate">
                {optionalImageLabelText(thumbnail, state.filenameThumb)}
              </span>
            </label>
            <School__InputGroupError.Jsx2
              message="must be a JPEG / PNG under 2 MB in size"
              active={state.invalidThumb}
            />
          </div>
          <div key="course-images-editor__cover" className="mt-4">
            <label
              className="block tracking-wide text-gray-800 text-xs font-semibold"
              htmlFor="sc-images-editor__logo-on-400-bg-input">
              {"Cover Image" |> str}
            </label>
            <input
              disabled={state.updating}
              className="hidden"
              name="course_cover"
              type_="file"
              accept=".jpg,.jpeg,.png,.gif,image/x-png,image/gif,image/jpeg"
              id="course-images-editor__cover"
              required=false
              multiple=false
              onChange={updateImage(send, true)}
            />
            <label
              className="file-input-label mt-2"
              htmlFor="course-images-editor__cover">
              <i className="fas fa-upload" />
              <span className="ml-2 truncate">
                {optionalImageLabelText(cover, state.filenameCover)}
              </span>
            </label>
            <School__InputGroupError.Jsx2
              message="must be a JPEG / PNG under 2 MB in size"
              active={state.invalidCover}
            />
          </div>
          <button
            type_="submit"
            key="sc-images-editor__update-button"
            disabled={updateButtonDisabled(state)}
            className="btn btn-primary btn-large mt-6">
            {updateButtonText(state.updating) |> str}
          </button>
        </DisablingCover.Jsx2>
      </form>
    </SchoolAdmin__EditorDrawer.Jsx2>;
  },
};
